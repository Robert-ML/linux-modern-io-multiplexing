#include <stdint.h>
#include <stdio.h>
#include <sys/poll.h>
#include <unistd.h>

#include "./pollfd_vector.h"
#include "../server_benchmark.h"
#include "../server_utils.h"
#include "../../common/debug.h"
#include "../../common/defines.h"
#include "../../common/measurements.h"
#include "../../common/time_utils.h"
#include "../../common/utils.h"


/* =========================================================================
 *  Structure definitions
 * ========================================================================= */

typedef struct pd_echo_connection {
    uint32_t buf_size;
    uint8_t buf[DEFAULT_SERVER_BUFFER_SIZE];
} pd_echo_connection_t;


/* =========================================================================
 *  Global variables
 * ========================================================================= */

static struct server_bench bench;
static struct pollfd_vector pollfdv;

/* =========================================================================
 *  Function declarations
 * ========================================================================= */

static void service_loop(const int listening_socket);
static void service_events(const int ls, int * const timer, const int no_events);
static void do_exit_cleanup(const int ls);

static void poll_add_fd(const int fd, const short int poll_events,
    void *private_data
);

static void timer_expired(const int timer_fd);
static void close_timer(const int timer_fd);

static void poll_handle_new_client(const int listening_socket);
/**
 * @brief: Treat events coming from clients.
 *
 * @param pollfdv_index: index in the global pollfdv to handle event for. No
 * bound checks are performed.
 *
 * @returns: If a client disconnected and was removed from the poll, returns 1
 * to know how to iterate safely over the remaining poll events.
 */
static int handle_client_event(const int pollfdv_index);
static void close_client(const int pollfdv_index);


int main(int, char **)
{
    int listening_socket;

    bench = sb_create(BENCH_EXPECTED_MESSAGES);
    pollfdv = pollfdv_create();
    register_signal_handler();

    printf("Poll Echo Server (PID: %d)\n--- Starting!\n\n", getpid());

    listening_socket = create_ipv4_listening_socket(SERVER_PORT);

    dlog(LOG_INFO, "listening socket fd: %d\n", listening_socket);

    service_loop(listening_socket);

    do_exit_cleanup(listening_socket);

    return 0;
}

static void service_loop(const int listening_socket)
{
    int rc;
    int no_events;
    int timer_fd = create_warmup_timer();

    // register the socket and timer
    poll_add_fd(listening_socket, POLLIN, NULL);
    poll_add_fd(timer_fd, POLLIN, NULL);

    while (server_running) {
        no_events = poll(pollfdv.data, pollfdv.size, -1);
        if (no_events < 0 && errno == EINTR) {
            // we just had a signal coming
            continue;
        } else if (errno != EINTR) {
            assert_nonn(no_events, "poll");
        }

        sb_requests_performed(&bench, no_events);
        dlog(LOG_DEBUG, "no_events: %d\n", no_events);

        service_events(listening_socket, &timer_fd, no_events);
    }

    sb_stop(&bench);
    sb_save_bench(&bench);

    if (timer_fd != -1) {
        rc = close(timer_fd);
        assert_zero(rc, "close");
    }
}

static void service_events(const int ls, int * const timer, const int no_events)
{
    uint32_t i;
    int removed_client;
    struct pollfd *ev;
    uint32_t size = pollfdv.size;
    int no_serviced = 0;

    for (i = 0; i < size && no_serviced < no_events; ++i) {
        ev = &pollfdv.data[i];

        if (ev->revents == 0) {
            continue;
        }

        ++no_serviced;

        if (ev->fd == ls && ev->revents & POLLIN) {
            // new client connection requested
            poll_handle_new_client(ls);
        } else if (ev->fd == *timer && ev->revents & POLLIN) {
            // timer event
            timer_expired(*timer);
            // sketchy operation to remove the current indexed elemnet
            pollfdv_remove_index(&pollfdv, i);
            --i; // recheck the current element after removal
            *timer = -1;
            continue;
        } else {
            // client event
            removed_client = handle_client_event(i);
            if (removed_client) {
                --i; // recheck the current element after removal
                continue;
            }
        }

        ev->revents = 0;
    }

    assert_zero(no_events - no_serviced, "Did not service all requests");
}

static void do_exit_cleanup(const int ls)
{
    int rc;
    uint32_t i;

    rc = close(ls);
    assert_zero(rc, "close");

    sb_free(&bench);

    // TODO: close the pd in pollfdv and free pollfdv
    for (i = 0; i < pollfdv.size; ++i) {
        free(pollfdv.pd[i]);
    }
    pollfdv_free(&pollfdv);
}


static void poll_add_fd(const int fd, const short int poll_events, void *private_data)
{
    struct pollfd config = {
        .fd = fd,
        .events = poll_events,
        .revents = 0
    };
    pollfdv_add(&pollfdv, config, private_data);
}

static void timer_expired(const int timer_fd)
{
    close_timer(timer_fd);

    dlog(LOG_INFO, "Starting to benchmark\n");
    sb_start(&bench);
}

static void close_timer(const int timer_fd)
{
    int rc;

    rc = close(timer_fd);
    assert_zero(rc, "close");
}


static void poll_handle_new_client(const int listening_socket)
{
    int client_fd;
    pd_echo_connection_t *con;

    client_fd = accept_client(listening_socket);

    // allocate the client connection structure
    con = (pd_echo_connection_t *)malloc(sizeof(*con));
    assert_nnull(con, "malloc");
    con->buf_size = 0;

    poll_add_fd(client_fd, POLLIN, con);

    sb_client_connected(&bench);
}

static int handle_client_event(const int index)
{
#if BENCH_MEASURE_SERVICING_LATENCY == 1
    struct timespec start, end;
#endif
    int read_bytes;
    pd_echo_connection_t * con;
    int removed_client = 0;

    if (pollfdv.data[index].revents & (POLLHUP | POLLERR)) {
        // connection closed
        removed_client = 1;
        close_client(index);
    } else if (pollfdv.data[index].revents & POLLIN) {
        // client sent data
#if BENCH_MEASURE_SERVICING_LATENCY == 1
        start = now_monotonic();
#endif

        con = (pd_echo_connection_t *)pollfdv.pd[index];
        read_bytes = read_n_echo(
            pollfdv.data[index].fd,
            con->buf,
            sizeof(con->buf)
        );

#if BENCH_MEASURE_SERVICING_LATENCY
        end = now_monotonic();
        sb_record_event(&bench, start, end);
#endif

        if (read_bytes == 0) {
            // the connection closed
            removed_client = 1;
            close_client(index);
        }
    } else {
        // unexpected
        dlog(LOG_NOTICE, "Unexpected else branch\n");
    }

    return removed_client;
}

static void close_client(const int index)
{
    int rc;

    rc = close(pollfdv.data[index].fd);
    assert_zero(rc, "close");

    free(pollfdv.pd[index]);

    pollfdv_remove_index(&pollfdv, index);
}
