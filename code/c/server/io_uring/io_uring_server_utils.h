#ifndef CODE_C_SERVER_IO_URING_IO_URING_SERVER_UTILS_H_
#define CODE_C_SERVER_IO_URING_IO_URING_SERVER_UTILS_H_

#include <arpa/inet.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <linux/io_uring.h>

#include "../../common/defines.h"


#define KERNEL_THREAD_IDLE_SLEEP_TIME_MS 3100U
#define KERNEL_THREAD_CPU_BIND 0U


/* =========================================================================
 *  Structure declarations
 * ========================================================================= */

struct iou_op_mem_tracker_node;
struct iou_op;


/* =========================================================================
 *  Structure definitions for connection tracking
 * ========================================================================= */

enum io_operation_types {
    ACCEPTED_CONNECTION,
    CLOSE_CONNECTION,
    RECV_FROM_CLIENT,
    SEND_TO_CLIENT,
    TIMER_EXPIRED,
};

struct client_con {
    int fd;
    int busy; // to not deallocate the object if the buffer is in the SQ_ring

    uint32_t buf_send_len;
    uint8_t buf[DEFAULT_SERVER_BUFFER_SIZE];

#if DO_SERVER_SIDE_BENCHMARKING == 1 && BENCH_MEASURE_SERVICING_LATENCY == 1
    // benchmarking members;
    struct timespec bench_start_send;
#endif
};


/* =========================================================================
 *  Structure definitions for io_uring SQEs
 * ========================================================================= */

struct iou_socket_accept {
    int fd;
    struct sockaddr_in addr;
    socklen_t addrlen;
};

/**
 * @brief: Structure to help create SQEs. It is passed to
 * `iou_config_and_submit` to create a IO Uring submission. It is also used to
 * be passed as the `user_data` pointer.
 */
struct iou_op {
    enum io_operation_types op; // to organize the request's state machine

    union {
        struct iou_socket_accept info_accept;
        struct client_con info_con;
        struct __kernel_timespec info_timer;
    };

    // could be replaced by usage of an Hash Map in struct iou_op_mem_tracker
    struct iou_op_mem_tracker_node *mem_tracker;
};


struct iou_op_mem_tracker_node {
    struct iou_op *allocated_op;

    struct iou_op_mem_tracker_node *next;
    struct iou_op_mem_tracker_node *prev;
};

struct iou_op_mem_tracker {
    struct iou_op_mem_tracker_node *head;
    struct iou_op_mem_tracker_node *tail;
};

/* =========================================================================
 *  Structure definitions for io_uring
 * ========================================================================= */

struct iou_sq_ring {
    unsigned int *head;
    unsigned int *tail;
    unsigned int *ring_mask;
    unsigned int *ring_entries;
    unsigned int *flags;
    unsigned int *array;
};

struct iou_cq_ring {
    unsigned int *head;
    unsigned int *tail;
    unsigned int *ring_mask;
    unsigned int *ring_entries;
    struct io_uring_cqe *cqes;
};

struct iou {
    int ring_fd;

    uint32_t ring_size;
    struct io_uring_params configuration;

    struct iou_sq_ring sq_ring;
    struct io_uring_sqe *sqes;

    struct iou_cq_ring cq_ring;

    void *sq_ptr_mapped;
    unsigned int sq_mapped_size;
    void *cq_ptr_mapped; // only populated if had to be mapped
    unsigned int cq_mapped_size;
    void *sqe_ptr_mapped;
    unsigned int sqe_mapped_size;
};


/* =========================================================================
 *  Function declarations for interracting with IO Uring
 * ========================================================================= */

struct iou iou_create(const uint32_t ring_size, const int use_kernel_pooling);
void iou_free(struct iou * const iou);
/**
 * @brief: Performs `io_enter`, taking into account if the ring is configured
 * in kernel thread poll mode. `sig_mask` is used as described in
 * `io_uring_enter(2)` man page.
 */
void iou_enter_or_wake(
    struct iou * const iou, const unsigned int to_submit,
    sigset_t * const sig_mask
);

/**
 * @brief: This function takes the desired operation and makes sure to submit
 * it configured to the IO Uring SQ_ring.
 *
 * The given `op` will be passed to the SQE as `user_data` to be given back in
 * the completion event. The user must ensure the object's lifetime if desires
 * to use it at the completion event moment.
 *
 * @return: 0 if it could submit it, otherwise a negative number of error code.
 */
int iou_config_and_submit(struct iou * const iou, struct iou_op * const op);

/**
 * @brief: Checks the completion queue, and if there is a completion event it
 *  populates `cqe_out` with the retrieved info.
 *
 * @param iou: IO Uring struct with the ring info.
 * @param cqe_out: CQE that is populated and returned if there is a completed
 * event.
 *
 * @return: `cqe_out` populated with the CQE if there was one, otherwise NULL
 * and `cqe_out` was untouched.
 */
struct io_uring_cqe * iou_get_cqe(
    struct iou * const iou, struct io_uring_cqe * const cqe_out
);

void io_uring_debug_print_rings(const struct iou * const iou);

/* =========================================================================
 *  Function declarations for interracting with the memory allocation tracker
 * ========================================================================= */

struct iou_op_mem_tracker mem_t_create(void);
void mem_t_push_back(
    struct iou_op_mem_tracker * const tracker,
    struct iou_op_mem_tracker_node * const node
);
void mem_t_remove_node(
    struct iou_op_mem_tracker * const tracker,
    struct iou_op_mem_tracker_node * const node
);


#endif /* CODE_C_SERVER_IO_URING_IO_URING_SERVER_UTILS_H_ */
