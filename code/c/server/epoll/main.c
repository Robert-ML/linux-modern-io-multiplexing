#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
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


/* =========================================================================
 *  Structure definitions
 * ========================================================================= */

typedef struct pd_echo_connection {
    int fd;
    uint32_t buf_size;
    uint8_t buf[DEFAULT_SERVER_BUFFER_SIZE];
    // double linked list to keep track of connections
    struct pd_echo_connection *next;
    struct pd_echo_connection *prev;
} pd_echo_connection_t;


/* =========================================================================
 *  Global variables
 * ========================================================================= */

static struct server_bench bench;
/**
 * Double linked list to keep track of connections
 */
static pd_echo_connection_t *dll_head = NULL;
static pd_echo_connection_t *dll_tail = NULL;


/* =========================================================================
 *  Function declarations
 * ========================================================================= */

static void service_loop(const int listening_socket);
static void service_events(
    const int epoll_fd, struct epoll_event * const evs, const int no_events,
    const int ls, struct pd_fd * const tfd_holder
);
static void do_service_loop_cleanup(
    const int epoll_fd, struct pd_fd * const tfd_holder
);
static void do_exit_cleanup(const int ls);

static int create_epoll(void);
/**
 * @brief: Add the listening socket to the epoll instance.
 *
 * @param epoll_fd: File descriptor of the epoll instance.
 * @param lfd_holder: Private data (containing the socket) given to epoll.
 */
static void epoll_add_listening_socket(const int epoll_fd,
    struct pd_fd * const lfd_holder
);

/**
 * @brief: Add the clock to the epoll instance.
 *
 * @param epoll_fd: File descriptor of the epoll instance.
 * @param tfd_holder: Private data (containing the clock fd) given to epoll.
 */
static void epoll_add_timer(const int epoll_fd,
    struct pd_fd * const tfd_holder
);
static void timer_expired(struct pd_fd * const tfd_holder);
static void close_timer(struct pd_fd * const tfd_holder);

static void epoll_handle_new_client(const int epoll_fd,
    const int listening_socket
);
static void handle_client_event(const struct epoll_event * const ev);
static void close_client(pd_echo_connection_t * const con);

/**
 * @brief: A constructed connection but not put in the double linked list is
 * connected at the back of the list and all links are initialized.
 */
static void dll_connect_back(pd_echo_connection_t * const con);
/**
 * @brief: A connection that is a node in the connection double linked list
 * is safely removed and all the neighboring nodes are sticked back together.
 */
static void dll_remove(pd_echo_connection_t * const con);


int main(int, char **)
{
    int listening_socket;

    bench = sb_create(BENCH_EXPECTED_MESSAGES);
    register_signal_handler();

    printf("Epoll Echo Server (PID: %d)\n--- Starting!\n\n", getpid());

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
    sigset_t block_mask, orig_mask;
    const int epoll_fd = create_epoll();
    struct pd_fd lfd_holder = { .fd = listening_socket };
    struct pd_fd tfd_holder = { .fd = create_warmup_timer() };
    struct epoll_event evs[EPOLL_WAIT_MAX_EVENTS];

    // initialize the signal masks
    rc = sigprocmask(SIG_BLOCK, NULL, &orig_mask);
    assert_zero(rc, "sigprocmask get orig_mask");
    rc = sigemptyset(&block_mask);
    assert_zero(rc, "sigemptyset");
    rc = sigaddset(&block_mask, SIGUSR1);
    assert_zero(rc, "sigaddset");

    // register the socket and timer
    epoll_add_listening_socket(epoll_fd, &lfd_holder);
    epoll_add_timer(epoll_fd, &tfd_holder);

    while (
        !(rc = sigprocmask(
            SIG_BLOCK, &block_mask, NULL
        )) && server_running
    ) {
        assert_zero(rc, "sigprocmask");

        no_events = epoll_pwait(
            epoll_fd, evs, sizeof(evs) / sizeof(evs[0]), -1, &orig_mask
        );
        sigprocmask(SIG_SETMASK, &orig_mask, NULL);

        if (no_events < 0 && errno == EINTR) {
            // we just had a signal coming
            continue;
        } else if (errno != EINTR) {
            assert_nonn(no_events, "epoll_wait");
        }

        sb_requests_performed(&bench, no_events);

        dlog(LOG_DEBUG, "no_events: %d\n", no_events);

        service_events(
            epoll_fd, evs, no_events, listening_socket, &tfd_holder
        );
    }

    sb_stop(&bench);
    sb_save_bench(&bench);

    do_service_loop_cleanup(epoll_fd, &tfd_holder);
}

static void service_events(
    const int epoll_fd, struct epoll_event * const evs, const int no_events,
    const int ls, struct pd_fd * const tfd_holder)
{
    int i;
    struct epoll_event *ev;

    for (i = 0; i < no_events; ++i) {
        ev = &(evs[i]);

        dlog(LOG_DEBUG, "fd: %d | event_type: %x\n",
            ((struct pd_fd *)ev->data.ptr)->fd,
            ev->events
        );

        if (((struct pd_fd *)ev->data.ptr)->fd == ls) {
            // new client connection requested
            epoll_handle_new_client(epoll_fd, ls);
        } else if (((struct pd_fd *)ev->data.ptr)->fd == tfd_holder->fd) {
            // timer event
            timer_expired(tfd_holder);
        } else {
            // client event
            handle_client_event(ev);
        }
    }
}

static void do_service_loop_cleanup(
        const int epoll_fd, struct pd_fd * const tfd_holder
)
{
    int rc;
    pd_echo_connection_t *p, *next;

    if (tfd_holder->fd != -1) {
        close_timer(tfd_holder);
    }

    rc = close(epoll_fd);
    assert_zero(rc, "close epoll_fd");

    p = dll_head;
    while (p) {
        next = p->next;
        close_client(p);
        p = next;
    }
}

static void do_exit_cleanup(const int ls)
{
    int rc;

    rc = close(ls);
    assert_zero(rc, "close");

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
    struct epoll_event ev = {
        .events = EPOLLIN | EPOLLET,
        .data.ptr = lfd_holder
    };

    // add the listening socket to the poll
    rc = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, lfd_holder->fd, &ev);
    assert_zero(rc, "epoll_ctl listening_socket");
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

static void timer_expired(struct pd_fd * const tfd_holder)
{
    close_timer(tfd_holder);

    dlog(LOG_INFO, "Starting to benchmark\n");
    sb_start(&bench);
}

static void close_timer(struct pd_fd * const tfd_holder)
{
    int rc;

    // this also removes it from the epoll instance if it is the only file
    // descriptor (see definition `close_client` for details)
    rc = close(tfd_holder->fd);
    assert_zero(rc, "close");

    tfd_holder->fd = -1;
}


static void epoll_handle_new_client(const int epoll_fd, const int listening_socket)
{
    int rc;
    int client_fd;
    struct epoll_event ev;
    pd_echo_connection_t *con;

    client_fd = accept_client(listening_socket);

    // allocate the client connection structure
    con = (pd_echo_connection_t *)calloc(1, sizeof(*con));
    assert_nnull(con, "calloc");
    con->fd = client_fd;
    con->buf_size = 0;

    // add the client to the epoll
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    ev.data.ptr = con;
    rc = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
    assert_zero(rc, "epoll_ctl client_fd");

    // add the client connection to the connection double linked list
    dll_connect_back(con);

    sb_client_connected(&bench);
}

static void handle_client_event(const struct epoll_event * const ev)
{
#if BENCH_MEASURE_SERVICING_LATENCY == 1
    struct timespec start, end;
#endif
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

#if BENCH_MEASURE_SERVICING_LATENCY == 1
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

    // remove from the client connection double linked list
    dll_remove(con);

    rc = close(con->fd);
    assert_zero(rc, "close");

    free(con);
}


static void dll_connect_back(pd_echo_connection_t * const con)
{
    if (dll_head == NULL) {
        dll_head = con;
        dll_tail = con;
    } else {
        con->prev = dll_tail;
        dll_tail = con;
    }
}

static void dll_remove(pd_echo_connection_t * const con)
{
    if (con->next) {
        con->next->prev = con->prev;
    }
    if (con->prev) {
        con->prev->next = con->next;
    }
    if (con == dll_head) {
        dll_head = con->next;
    }
    if (con == dll_tail) {
        dll_tail = con->prev;
    }
}
