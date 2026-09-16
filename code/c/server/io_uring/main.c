#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "./io_uring_server_utils.h"
#include "../server_benchmark.h"
#include "../server_utils.h"
#include "../../common/debug.h"
#include "../../common/defines.h"
#include "../../common/measurements.h"
#include "../../common/time_utils.h"
#include "../../common/utils.h"


/* =========================================================================
 *  Defines
 * ========================================================================= */
#ifndef IO_URING_RING_SIZE
#error "Not defined the number of entries in the IO Uring ring queue"
#endif

#ifndef IO_URING_RUN_IN_KERNEL_POLL_MODE
#error "Define if IO Uring should run in kernel poll mode (1 or 0)"
#endif

/* Announce CQE consumption in batches */
#define IO_URING_CQE_BATCHING 256U
/* Announce SQE submission in batches */
#define IO_URING_SQE_BATCHING 256U


/* =========================================================================
 *  Global variables
 * ========================================================================= */

static struct server_bench bench;
static struct iou_op_mem_tracker mem_allocs_tracker;


/* =========================================================================
 *  Function declarations
 * ========================================================================= */

static void service_loop(const int listening_socket);
static int service_cqe_events(
    struct iou * const iou, const struct io_uring_cqe * const cqe
);
static void do_service_loop_cleanup(struct iou * const iou);
static void do_exit_cleanup(const int ls);

/**
 * @brief: Consumes CQEs if batching demanded or we are forcing it.
 *
 * @param iou: IO Uring struct with the ring info.
 * @param io_events_completed: How many io events have completed. Tries to
 * consume that many from the CQE ring if it reached / surpassed the batching
 * limit or we forced it.
 * @param force_consumption: Bool to force `io_events_completed` to be consumed
 * from the CQE ring.
 *
 * @returns: 0 if any events were consumed, or the previous
 * `io_events_completed`.
*/
static int handle_cqe_batching(
    struct iou * const iou, const unsigned int io_events_completed,
    const int force_consumption
);

/**
 * @brief: Checks how many SQEs are pending and if batching demands to submit
 * them.
 *
 * @param iou: IO Uring struct with the ring info.
*/
static void handle_sqe_batching(struct iou * const iou);

static int create_accept_sqe(
    struct iou * const iou, struct iou_op * const allocated_op,
    const int listening_socket
);
/* Disabled timer that starts benchmarking inside the server */
// static int create_timer_sqe(
//     struct iou * const iou, struct iou_op * const allocated_op
// );

static int handle_cqe_accept(
    struct iou * const iou, struct iou_op * const accept_req, const int result
);
static int handle_cqe_close(
    struct iou * const iou, struct iou_op * const close_req, const int result
);
static int handle_cqe_recv(
    struct iou * const iou, struct iou_op * const recv_req, const int result
);
static int handle_cqe_send(
    struct iou * const iou, struct iou_op * const send_req, const int result
);
static int handle_cqe_timer(
    struct iou * const iou, struct iou_op * const timer_req, const int result
);

static void handle_new_client(struct iou * const iou, const int socket);
static int prep_close_client_req(
    struct iou * const iou, struct iou_op * const op_w_info_con
);
static void handle_closing_client(struct iou_op * const w_info_con);


int main(int, char **)
{
    int listening_socket;

    bench = sb_create(BENCH_EXPECTED_MESSAGES);
    mem_allocs_tracker = mem_t_create();
    register_signal_handler();

    printf("IO_uring Echo Server (PID: %d)\n--- Starting!\n\n", getpid());

    listening_socket = create_ipv4_listening_socket(SERVER_PORT);

    dlog(LOG_INFO, "listening socket fd: %d\n", listening_socket);

    service_loop(listening_socket);

    do_exit_cleanup(listening_socket);

    return 0;
}

static void service_loop(const int listening_socket)
{
    int io_events_completed = 0;
    struct io_uring_cqe cqe_storage, *cqe;
    struct iou iou = iou_create(
        IO_URING_RING_SIZE,
        IO_URING_RUN_IN_KERNEL_POLL_MODE
    );

    struct iou_op accept_req = { .mem_tracker = NULL };
    create_accept_sqe(&iou, &accept_req, listening_socket);

    /* Disabled timer that starts benchmarking inside the server */
    // struct iou_op timer_req = { .mem_tracker = NULL };
    // create_timer_sqe(&iou, &timer_req);

    while (server_running) {
        cqe = iou_get_cqe_peek(&iou, io_events_completed, &cqe_storage);

        // check if we do not have any more completed events
        if (cqe == NULL) {
            io_events_completed = handle_cqe_batching(&iou, io_events_completed, 1);

            iou_enter_or_wake(&iou, 0);
            continue;
        }

        io_events_completed += 1;
        io_events_completed = handle_cqe_batching(&iou, io_events_completed, 0);

        dlog(LOG_DEBUG, "Got CQE: user_data: %p | op: %d\n", (void*)cqe->user_data, ((struct iou_op *)cqe->user_data)->op);

        service_cqe_events(&iou, cqe);

        // check if we generated enough pending SQEs to submit them
        handle_sqe_batching(&iou);
    }

    sb_stop(&bench);
    sb_save_bench(&bench);

    do_service_loop_cleanup(&iou);
}

static int service_cqe_events(
    struct iou * const iou, const struct io_uring_cqe * const cqe
)
{
    struct iou_op * const op = (struct iou_op * const)cqe->user_data;

    sb_requests_performed(&bench, 1);

    switch (op->op)
    {
    case ACCEPTED_CONNECTION:
        return handle_cqe_accept(iou, op, cqe->res);

    case CLOSE_CONNECTION:
        return handle_cqe_close(iou, op, cqe->res);

    case RECV_FROM_CLIENT:
        return handle_cqe_recv(iou, op, cqe->res);

    case SEND_TO_CLIENT:
        return handle_cqe_send(iou, op, cqe->res);

    case TIMER_EXPIRED:
        return handle_cqe_timer(iou, op, cqe->res);

    default:
        dlog(LOG_WARNING, "Hit default case for operation: %d\n", op->op);
        return 0;
    }
}

static void do_service_loop_cleanup(struct iou * const iou)
{
    int rc;
    struct iou_op_mem_tracker_node *next;
    struct iou_op_mem_tracker_node *tracker = mem_allocs_tracker.head;

    // close and free the ring
    iou_free(iou);

    // close the connections synchronously and free the memory
    while (tracker) {
        next = tracker->next;

        rc = close(tracker->allocated_op->info_con.fd);
        assert_zero(rc, "close(client_socket)");

        free(tracker->allocated_op);
        free(tracker);

        tracker = next;
    }

    mem_allocs_tracker.head = NULL;
    mem_allocs_tracker.tail = NULL;
}

static void do_exit_cleanup(const int ls)
{
    int rc;

    rc = close(ls);
    assert_zero(rc, "close");

    sb_free(&bench);
}


/* =========================================================================
 * Definitions for helpers to init the service loop
 * ========================================================================= */

static int create_accept_sqe(
    struct iou * const iou, struct iou_op * const allocated_op,
    const int listening_socket
)
{
    int rc;
    struct iou_op * const accept_req = allocated_op;

    accept_req->op = ACCEPTED_CONNECTION;
    accept_req->info_accept.fd = listening_socket;
    accept_req->info_accept.addrlen = sizeof(accept_req->info_accept.addr);

    rc = -iou_prep_from_op(iou, accept_req);
    io_uring_assert_zero(rc, "iou_prep_from_op(accept_req)");

    return 1;
}

/* Disabled timer that starts benchmarking inside the server */
// static int create_timer_sqe(
//     struct iou * const iou, struct iou_op * const allocated_op
// )
// {
//     int rc;
//     struct iou_op * const timer_req = allocated_op;

//     timer_req->op = TIMER_EXPIRED;
//     timer_req->info_timer.tv_sec = 10;
//     timer_req->info_timer.tv_nsec = 0;

//     rc = iou_prep_from_op(iou, timer_req);
//     io_uring_assert_zero(rc, "iou_prep_from_op(timer_req)");

//     return 1;
// }

static int handle_cqe_batching(
    struct iou * const iou, const unsigned int io_events_completed,
    const int force_consumption
)
{
    if (
        (io_events_completed > IO_URING_CQE_BATCHING)
        || (force_consumption && io_events_completed)
    ) {
        iou_cqe_consume(iou, io_events_completed);
        return 0;
    }

    return io_events_completed;
}

static void handle_sqe_batching(struct iou * const iou)
{
    unsigned int pending = iou_get_no_pending_sqes(iou);

    if (pending <= IO_URING_SQE_BATCHING) {
        return;
    }

    iou_enter_or_wake(iou, 0);
}

/* =========================================================================
 *  Definitions CQE handlers
 * ========================================================================= */

static int handle_cqe_accept(
    struct iou * const iou, struct iou_op * const accept_req, const int result
)
{
    int rc;
    io_uring_assert_nonn(result, "CQE accept");

    const int client_socket = result;
    handle_new_client(iou, client_socket);

    // resubmit the accept operation
    rc = iou_prep_from_op(iou, accept_req);
    io_uring_assert_zero(rc, "iou_prep_from_op(accept_req)");

    return 2;
}

static int handle_cqe_close(
    struct iou *, struct iou_op * const close_req, const int result
)
{
    io_uring_assert_zero(result, "CQE close");
    close_req->info_con.fd = -1;
    handle_closing_client(close_req);

    return 0;
}

static int handle_cqe_recv(
    struct iou * const iou, struct iou_op * const recv_req, const int result
)
{
    int rc;
    struct iou_op *send_req;

    // first mark the connection as not busy anymore
    recv_req->info_con.busy = 0;

    if (result == -ECONNRESET) {
        dlog(LOG_INFO, "recv: connection reset by peer\n");
        return prep_close_client_req(iou, recv_req);
    }
    io_uring_assert_nonn(result, "CQE recv");

    const unsigned int read_bytes = result;

    if (read_bytes == 0) {
        // Received EOF, close client
        return prep_close_client_req(iou, recv_req);
    } else if (read_bytes > DEFAULT_BUFFER_SIZE) {
        dlog(LOG_CRIT, "recv: got more than expected");
        exit(EXIT_FAILURE);
    } else if (read_bytes < DEFAULT_BUFFER_SIZE) {
        dlog(LOG_CRIT, "recv: got less than expected");
        exit(EXIT_FAILURE);
    }

    // benchmark the start of the send part of the echo
    recv_req->info_con.bench_start_send = now_monotonic();

    // we repurpose the structure as we always do a send after recv
    send_req = recv_req;

    // prep the request for sending (echo back)
    send_req->op = SEND_TO_CLIENT;
    send_req->info_con.buf_send_len = read_bytes;
    send_req->info_con.busy = 1;

    rc = iou_prep_from_op(iou, send_req);
    io_uring_assert_zero(rc, "iou_prep_from_op(send_req)");

    return 1;
}

static int handle_cqe_send(
    struct iou * const iou, struct iou_op * const send_req, const int result
)
{
    int rc;
    struct iou_op *recv_req;
    struct timespec bench_end_send;

    // first mark the connection as not busy anymore
    send_req->info_con.busy = 0;

    io_uring_assert_nonn(result, "CQE send");

    const unsigned int sent_bytes = result;
    if (sent_bytes < DEFAULT_BUFFER_SIZE) {
        dlog(LOG_CRIT, "send: less than expected");
        exit(EXIT_FAILURE);
    }

    // benchmark the end of the send part of the echo
    bench_end_send = now_monotonic();
    sb_record_event(
        &bench, send_req->info_con.bench_start_send, bench_end_send
    );

    // we repurpose the structure as we always expect a recv after the send
    recv_req = send_req;

    // prep the request to wait for more data
    recv_req->op = RECV_FROM_CLIENT;
    recv_req->info_con.busy = 1;

    rc = iou_prep_from_op(iou, recv_req);
    io_uring_assert_zero(rc, "iou_prep_from_op(recv_req)");

    return 1;
}

static int handle_cqe_timer(
    struct iou * const, struct iou_op * const, const int result
)
{
    io_uring_assert(result == -ETIME, result, "CQE timer");

    dlog(LOG_INFO, "Starting to benchmark\n");
    sb_start(&bench);

    return 0;
}


/* =========================================================================
 *  Definitions for helpers of CQE handlers
 * ========================================================================= */

static void handle_new_client(struct iou * const iou, const int socket)
{
    int rc;
    struct iou_op_mem_tracker_node *tracker = calloc(1, sizeof(*tracker));
    assert_nnull(tracker, "calloc tracker");

    struct iou_op *recv_req;
    struct iou_op *client_op = calloc(1, sizeof(*client_op));
    assert_nnull(client_op, "calloc client_op");

    // configure the connection information
    client_op->info_con.fd = socket;

    // create the double link between the object and the tracker
    client_op->mem_tracker = tracker;
    tracker->allocated_op = client_op;

    // add the tracker in the linked list
    mem_t_push_back(&mem_allocs_tracker, tracker);

    // prep the request for reading (and wait for data)
    recv_req = client_op;
    recv_req->op = RECV_FROM_CLIENT;
    recv_req->info_con.busy = 1;

    rc = iou_prep_from_op(iou, recv_req);
    io_uring_assert_zero(rc, "iou_prep_from_op(recv_req)");

    sb_client_connected(&bench);
}

static int prep_close_client_req(
    struct iou * const iou, struct iou_op * const op_w_info_con
)
{
    int rc;
    struct iou_op * const close_req = op_w_info_con;

    close_req->op = CLOSE_CONNECTION;

    rc = iou_prep_from_op(iou, close_req);
    io_uring_assert_zero(rc, "iou_prep_from_op(close_req)");

    return 1;
}

static void handle_closing_client(struct iou_op * const op_w_info_con)
{
    struct iou_op_mem_tracker_node *tracker = op_w_info_con->mem_tracker;

    mem_t_remove_node(&mem_allocs_tracker, tracker);

    free(tracker);
    free(op_w_info_con);
}
