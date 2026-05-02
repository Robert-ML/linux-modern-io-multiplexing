#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#include "../server_benchmark.h"
#include "../server_utils.h"
#include "../../common/debug.h"
#include "../../common/defines.h"
#include "../../common/measurements.h"
#include "../../common/time_utils.h"
#include "../../common/utils.h"


#define EPOLL_WAIT_MAX_EVENTS 256


typedef struct pd_echo_connection {
    int fd;
    uint32_t buf_size;
    uint8_t buf[DEFAULT_SERVER_BUFFER_SIZE];
} pd_echo_connection_t;


/* ================================================================
 *  Benchmarking variables
 * ================================================================ */
struct server_bench bench;


/* ================================================================
 *  Function declarations
 * ================================================================ */
static void service_loop(const int listening_socket);

static int create_epoll(void);
/**
 * @brief: Add the listening socket to the epoll instance.
 *
 * @param epoll_fd: File descriptor of the epoll instance.
 * @param lfd_holder: Private data (containing the socket) given to epoll.
 */
static void epoll_add_listening_socket(const int epoll_fd,
    struct pd_fd * const lfd_holder);

static int create_timer(void);
/**
 * @brief: Add the clock to the epoll instance.
 *
 * @param epoll_fd: File descriptor of the epoll instance.
 * @param tfd_holder: Private data (containing the clock fd) given to epoll.
 */
static void epoll_add_timer(const int epoll_fd,
    struct pd_fd * const tfd_holder);
static void timer_expired(const struct pd_fd * const tfd_holder);
static void close_timer(const struct pd_fd * const tfd_holder);

static void epoll_handle_new_client(const int epoll_fd,
    const int listening_socket);
static void handle_client_event(const struct epoll_event * const ev);
static void close_client(pd_echo_connection_t * const con);


int main(int, char **)
{
    int listening_socket;

    bench = sb_create(BENCH_EXPECTED_MESSAGES);
    register_signal_handler();

    printf("Epoll Echo Server (PID: %d)\n--- Starting!\n\n", getpid());

    listening_socket = create_ipv4_listening_socket(SERVER_PORT);

    dlog(LOG_INFO, "listening socket fd: %d\n", listening_socket);

    service_loop(listening_socket);

    return 0;
}


static void service_loop(const int listening_socket)
{
    int i;
    int no_events;
    const int epoll_fd = create_epoll();
    int timer_fd = create_timer();
    struct pd_fd lfd_holder = { .fd = listening_socket };
    struct pd_fd tfd_holder = { .fd = timer_fd };
    struct epoll_event evs[EPOLL_WAIT_MAX_EVENTS];
    struct epoll_event *ev;

    // register the socket and timer
    epoll_add_listening_socket(epoll_fd, &lfd_holder);
    epoll_add_timer(epoll_fd, &tfd_holder);

    while (server_running) {
        no_events = epoll_wait(epoll_fd, evs, sizeof(evs) / sizeof(evs[0]), -1);
        if (no_events < 0 && errno == EINTR) {
            // we just had a signal coming
            continue;
        } else if (errno != EINTR) {
            assert_nonn(no_events, "epoll_wait");
        }

        sb_requests_performed(&bench, no_events);

        dlog(LOG_INFO, "no_events: %d\n", no_events);

        for (i = 0; i < no_events; ++i) {
            ev = &(evs[i]);

            dlog(LOG_DEBUG, "fd: %d | event_type: %x\n",
                ((struct pd_fd *)ev->data.ptr)->fd,
                ev->events
            );

            if (((struct pd_fd *)ev->data.ptr)->fd == listening_socket) {
                // new client connection requested
                epoll_handle_new_client(epoll_fd, listening_socket);
            } else if (((struct pd_fd *)ev->data.ptr)->fd == timer_fd) {
                // timer event
                timer_expired(&tfd_holder);
                timer_fd = -1; // invalidate fd
            } else {
                // client event
                handle_client_event(ev);
            }
        }
    }

    sb_stop(&bench);
    sb_save_bench(&bench);
    sb_free(&bench);
}


static int create_epoll(void)
{
    const int epoll_fd = epoll_create1(0);
    assert_nonn(epoll_fd, "epoll_create1");
    return epoll_fd;
}

static void epoll_add_listening_socket(const int epoll_fd, struct pd_fd * const lfd_holder)
{
    int rc;
    struct epoll_event ev;

    // add the listening socket to the poll
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = lfd_holder;
    rc = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, lfd_holder->fd, &ev);
    assert_zero(rc, "epoll_ctl listening_socket");
}


static int create_timer(void)
{
    int rc;
    struct itimerspec settings;
    const int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);

    assert_nonn(timer_fd, "timerfd_create");

    // set timer
    memset(&settings, 0, sizeof(settings));
    settings.it_value.tv_sec = SERVER_WARMUP_TIME_S;

    rc = timerfd_settime(timer_fd, 0, &settings, NULL);
    assert_zero(rc, "timerfd_settime");

    return timer_fd;
}

static void epoll_add_timer(const int epoll_fd, struct pd_fd * const tfd_holder)
{
    int rc;
    struct epoll_event ev;

    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = tfd_holder;
    rc = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tfd_holder->fd, &ev);
    assert_zero(rc, "epoll_ctl timer add");
}

static void timer_expired(const struct pd_fd * const tfd_holder)
{
    close_timer(tfd_holder);

    dlog(LOG_INFO, "Starting to benchmark\n");
    sb_start(&bench);
}

static void close_timer(const struct pd_fd * const tfd_holder)
{
    int rc;

    // this also removes it from the epoll instance if it is the only file
    // descriptor (see definition `close_client` for details)
    rc = close(tfd_holder->fd);
    assert_zero(rc, "close");
}


static void epoll_handle_new_client(const int epoll_fd, const int listening_socket)
{
    int rc;
    int client_fd;
    struct epoll_event ev;
    pd_echo_connection_t *con;

    client_fd = accept_client(listening_socket);

    // allocate the client connection structure
    con = (pd_echo_connection_t *)malloc(sizeof(*con));
    assert_nnull(con, "malloc");
    con->fd = client_fd;

    // add the client to the epoll
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    ev.data.ptr = con;
    rc = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
    assert_zero(rc, "epoll_ctl client_fd");

    sb_client_connected(&bench);
}

static void handle_client_event(const struct epoll_event * const ev)
{
    struct timespec start, end;
    pd_echo_connection_t * const con = (pd_echo_connection_t *)ev->data.ptr;

    if (ev->events & EPOLLRDHUP) {
        // client closed the connection
        close_client(con);
    } else if (ev->events & EPOLLIN) {
        // client sent data
#if BENCH_MEASURE_SERVICING_LATENCY == 1
        start = now_monotonic();
#endif

        con->buf_size = read_n_echo(con->fd, con->buf, sizeof(con->buf));

        #if BENCH_MEASURE_SERVICING_LATENCY
        end = now_monotonic();
        sb_record_event(&bench, start, end);
#endif
    } else {
        // unexpected
        dlog(LOG_NOTICE, "Unexpected else branch\n");
        return;
    }
}

/*
 * Closing a file descriptor will cause it to be removed from all `epoll`
 * interest lists, but only if it is the last file descriptor referencing the
 * underlying file description structure. If `dup` or `fork` were used in the
 * codebase, this might get tricky. See man epoll(7) for more info.
 */
static void close_client(pd_echo_connection_t * const con)
{
    int rc;

    rc = close(con->fd);
    assert_zero(rc, "close");

    free(con);
}
