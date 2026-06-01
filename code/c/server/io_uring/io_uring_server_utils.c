#include "./io_uring_server_utils.h"

#include <signal.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <linux/io_uring.h>

#include "../../common/debug.h"
#include "../../common/utils.h"


/* =========================================================================
 *  Structure definitions for io_uring SQEs
 * ========================================================================= */

struct iou_acquired_sqe_data {
    struct io_uring_sqe *sqe;
    unsigned int tail;
    unsigned int index;
};


/* =========================================================================
 *  Syscall wrappers for io_uring since glibc doesn't have them
 * ========================================================================= */

static int io_uring_setup(unsigned entries, struct io_uring_params *p)
{
    int ret;
    ret = syscall(__NR_io_uring_setup, entries, p);
    return (ret < 0) ? -errno : ret;
}

static int io_uring_enter(int ring_fd, unsigned int to_submit,
    unsigned int min_complete, unsigned int flags, sigset_t *sig_mask
)
{
    int ret;
    ret = syscall(
        __NR_io_uring_enter, ring_fd, to_submit, min_complete, flags,
        sig_mask, _NSIG / 8
    );
    return (ret < 0) ? -errno : ret;
}


/* =========================================================================
 *  Definition and helpers for `iou_create`
 * ========================================================================= */

static struct io_uring_params iou_config_params(const int use_kernel_pooling)
{
    struct io_uring_params p;

    memset(&p, 0, sizeof(p));

    // the server is single threaded, only one thread (maybe the kernel) submits
    p.flags |= IORING_SETUP_SINGLE_ISSUER;

    if (!use_kernel_pooling) {
        // do not get interrupted when a CE appears
        p.flags |= IORING_SETUP_COOP_TASKRUN;

        return p;
    }

    // make a kernel thread poll the submission queue
    p.flags |= IORING_SETUP_SQPOLL;
    p.sq_thread_idle = KERNEL_THREAD_IDLE_SLEEP_TIME_MS;
    // set the CPU affinity of the kernel thread
    p.flags |= IORING_SETUP_SQ_AFF;
    p.sq_thread_cpu = KERNEL_THREAD_CPU_BIND;

    return p;
}

static void iou_check_features(
        const struct io_uring_params p, const int use_kernel_pooling
)
{
    if (!use_kernel_pooling) {
        return;
    }

    if (!(p.features & IORING_FEAT_SQPOLL_NONFIXED)) {
        dlog(LOG_ERR, "Necessary feature IORING_FEAT_SQPOLL_NONFIXED not supported\n");
        exit(EXIT_FAILURE);
    }
}

static struct iou iou_mmap_rings(const int ring_fd, const struct io_uring_params p, const uint32_t ring_size)
{
    struct iou iou = {
        .ring_fd = ring_fd,
        .ring_size = ring_size,
        .configuration = p,
        .sq_ptr_mapped = NULL,
        .sq_mapped_size = 0,
        .cq_ptr_mapped = NULL,
        .cq_mapped_size = 0,
        .sqe_ptr_mapped = NULL,
        .sqe_mapped_size = 0,
    };
    void *sqes;
    void *sq_ptr, *cq_ptr;
    unsigned int sqe_mapped_size;
    unsigned int sring_size = p.sq_off.array + p.sq_entries * sizeof(unsigned int);
    unsigned int cring_size = p.cq_off.cqes + p.cq_entries * sizeof(
        struct io_uring_cqe
    );

    if (p.features & IORING_FEAT_SINGLE_MMAP) {
        if (cring_size > sring_size) {
            sring_size = cring_size;
        }
        cring_size = sring_size;
    }

    // Map submission and completion queues
    sq_ptr = mmap(NULL, sring_size, PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_POPULATE, ring_fd, IORING_OFF_SQ_RING
    );
    assert_zero(sq_ptr == MAP_FAILED, "mmap submission queue");
    iou.sq_ptr_mapped = sq_ptr;
    iou.sq_mapped_size = sring_size;

    if (p.features & IORING_FEAT_SINGLE_MMAP) {
        cq_ptr = sq_ptr;
    } else {
        cq_ptr = mmap(NULL, cring_size, PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_POPULATE, ring_fd, IORING_OFF_CQ_RING);
        assert_zero(cq_ptr == MAP_FAILED, "mmap completion queue");
        iou.cq_ptr_mapped = cq_ptr;
        iou.cq_mapped_size = cring_size;
    }

    // Map submission queue entries array
    sqe_mapped_size = p.sq_entries * sizeof(struct io_uring_sqe);
    sqes = mmap(
        NULL, sqe_mapped_size,
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, ring_fd,
        IORING_OFF_SQES
    );
    assert_zero(sqes == MAP_FAILED, "mmap submission queue entries");
    iou.sqe_ptr_mapped = sqes;
    iou.sqe_mapped_size = sqe_mapped_size;

    // Complete the configuration and return the ring information
    iou.sq_ring.head = sq_ptr + p.sq_off.head;
    iou.sq_ring.tail = sq_ptr + p.sq_off.tail;
    iou.sq_ring.ring_mask = sq_ptr + p.sq_off.ring_mask;
    iou.sq_ring.ring_entries = sq_ptr + p.sq_off.ring_entries;
    iou.sq_ring.flags = sq_ptr + p.sq_off.flags;
    iou.sq_ring.array = sq_ptr + p.sq_off.array;

    iou.sqes = sqes;

    iou.cq_ring.head = cq_ptr + p.cq_off.head;
    iou.cq_ring.tail = cq_ptr + p.cq_off.tail;
    iou.cq_ring.ring_mask = cq_ptr + p.cq_off.ring_mask;
    iou.cq_ring.ring_entries = cq_ptr + p.cq_off.ring_entries;
    iou.cq_ring.cqes = cq_ptr + p.cq_off.cqes;

    return iou;
}

struct iou iou_create(const uint32_t ring_size, const int use_kernel_pooling)
{
    int ring_fd;
    struct io_uring_params p = iou_config_params(use_kernel_pooling);


    ring_fd = io_uring_setup(ring_size, &p);
    if (ring_fd < 0) {
        dlog(LOG_ERR, "io_uring_setup failed, [ERRNO]: %s\n", strerror(-ring_fd));
        exit(EXIT_FAILURE);
    }
    iou_check_features(p, use_kernel_pooling);

    return iou_mmap_rings(ring_fd, p, ring_size);
}


/* =========================================================================
 *  Definition for `iou_free`
 * ========================================================================= */

void iou_free(struct iou * const iou)
{
    int rc;

    // do the unmapping first
    rc = munmap(iou->sq_ptr_mapped, iou->sq_mapped_size);
    assert_zero(rc, "munmap(sq_ring)");

    if (iou->cq_ptr_mapped) {
        rc = munmap(iou->cq_ptr_mapped, iou->cq_mapped_size);
        assert_zero(rc, "munmap(cq_ring)");
    }

    rc = munmap(iou->sqe_ptr_mapped, iou->sqe_mapped_size);
    assert_zero(rc, "munmap(sqe_array)");

    // close the ring
    rc = close(iou->ring_fd);
    assert_zero(rc, "close(ring_fd)");

    // invalidate the structure's fields
    memset(iou, 0, sizeof(*iou));
    iou->ring_fd = -1;
}


/* =========================================================================
 *  Definition for `iou_enter_or_wake`
 * ========================================================================= */

void iou_enter_or_wake(
    struct iou * const iou, const unsigned int to_submit,
    sigset_t * const sig_mask
)
{
    int rc;
    unsigned int flags;

    // run blocking io_uring_enter and set sigmask
    if (!(iou->configuration.flags & IORING_SETUP_SQPOLL)) {
        rc = io_uring_enter(
            iou->ring_fd, to_submit, 1, IORING_ENTER_GETEVENTS, sig_mask
        );
        io_uring_assert(
            (rc >= 0) || (rc == -EINTR), rc,
            "io_uring_enter(IORING_ENTER_GETEVENTS)"
        );
        return;
    }

    // wake the SQ processing kernel thread if needed
    flags = atomic_load_explicit(iou->sq_ring.flags, memory_order_relaxed);
    if (flags & IORING_SQ_NEED_WAKEUP) {
        rc = io_uring_enter(iou->ring_fd, 0, 0, IORING_ENTER_SQ_WAKEUP, NULL);
        io_uring_assert(
            (rc >= 0) || (rc == -EINTR), rc,
            "io_uring_enter(IORING_ENTER_SQ_WAKEUP)"
        );
    }
}


/* =========================================================================
 *  Definition and helpers for `iou_config_and_submit`
 * ========================================================================= */

/**
 * @brief: Tries to acquire an SQE entry from the ring.
 *
 * @return: The acquired SQE wrapped with the tail and index info, which are to
 * be later used in the submission part. If it was not possible to acquire an
 * SQE, NULL is returned in it's field.
 */
static struct iou_acquired_sqe_data iou_acquire_sqe(struct iou * const iou)
{
    struct iou_acquired_sqe_data sqe_acquired = {
        .sqe = NULL,
    };

    sqe_acquired.tail = *iou->sq_ring.tail;

    if (sqe_acquired.tail - atomic_load_explicit(iou->sq_ring.head, memory_order_acquire) >= iou->ring_size) {
        return sqe_acquired;
    }

    // extract an available SQE from the ring
    sqe_acquired.index = sqe_acquired.tail & (*iou->sq_ring.ring_mask);

    sqe_acquired.sqe = &iou->sqes[sqe_acquired.index];
    memset(sqe_acquired.sqe, 0, sizeof(*sqe_acquired.sqe));

    return sqe_acquired;
}

static void iou_submit_sqe(
    struct iou * const iou, const struct iou_acquired_sqe_data sqe_acquired
)
{
    iou->sq_ring.array[sqe_acquired.index] = sqe_acquired.index;
    atomic_store_explicit(iou->sq_ring.tail, sqe_acquired.tail + 1, memory_order_release);
}

static void iou_prep_accept_sqe(
    struct iou_socket_accept * const info_accept,
    struct io_uring_sqe * const sqe
)
{
    sqe->opcode = IORING_OP_ACCEPT;
    sqe->fd = info_accept->fd;
    sqe->addr = (unsigned long long) &info_accept->addr;
    sqe->addr2 = (unsigned long long) &info_accept->addrlen;
    sqe->accept_flags = SOCK_NONBLOCK;
}

static void iou_prep_close_sqe(
    struct client_con * const con, struct io_uring_sqe * const sqe
)
{
    sqe->opcode = IORING_OP_CLOSE;
    sqe->fd = con->fd;
    sqe->file_index = 0; // no direct descriptors
}

static void iou_prep_recv_sqe(
    struct client_con * const con, struct io_uring_sqe * const sqe
)
{
    sqe->opcode = IORING_OP_RECV;
    sqe->fd = con->fd;
    sqe->addr = (unsigned long long) con->buf;
    sqe->len = sizeof(con->buf);
    sqe->msg_flags = 0;
}

static void iou_prep_send_sqe(
    struct client_con * const con, struct io_uring_sqe * const sqe
)
{
    sqe->opcode = IORING_OP_SEND;
    sqe->fd = con->fd;
    sqe->addr = (unsigned long long) con->buf;
    sqe->len = con->buf_send_len;
    sqe->msg_flags = 0;
}

static void iou_prep_timer_sqe(
    struct __kernel_timespec * const timer, struct io_uring_sqe * const sqe
)
{
    sqe->opcode = IORING_OP_TIMEOUT;
    sqe->addr = (unsigned long long) timer;
    sqe->len = 1;
    sqe->timeout_flags = 0;
    sqe->off = 0;
}

int iou_config_and_submit(struct iou * const iou, struct iou_op * const op)
{
    struct iou_acquired_sqe_data sqe_acquired = iou_acquire_sqe(iou);

    if (sqe_acquired.sqe == NULL) {
        return -EBUSY;
    }

    // store the user data
    sqe_acquired.sqe->user_data = (unsigned long long) op;

    // further configure the SQE according to the op
    switch (op->op)
    {
    case ACCEPTED_CONNECTION:
        iou_prep_accept_sqe(&op->info_accept, sqe_acquired.sqe);
        break;

    case CLOSE_CONNECTION:
        iou_prep_close_sqe(&op->info_con, sqe_acquired.sqe);
        break;

    case RECV_FROM_CLIENT:
        iou_prep_recv_sqe(&op->info_con, sqe_acquired.sqe);
        break;

    case SEND_TO_CLIENT:
        iou_prep_send_sqe(&op->info_con, sqe_acquired.sqe);
        break;

    case TIMER_EXPIRED:
        iou_prep_timer_sqe(&op->info_timer, sqe_acquired.sqe);
        break;

    default:
        dlog(LOG_WARNING, "Operation not treated in switch: %d\n", op->op);
        return -EINVAL;
    }

    iou_submit_sqe(iou, sqe_acquired);

    return 0;
}


/* =========================================================================
 *  Definition for `iou_get_cqe`
 * ========================================================================= */

struct io_uring_cqe * iou_get_cqe(
    struct iou * const iou, struct io_uring_cqe * const cqe_out
)
{
    struct io_uring_cqe *cqe_extracted;
    unsigned int head, index;

    head = *iou->cq_ring.head;

    // check there is a completed event
    if (head == atomic_load_explicit(iou->cq_ring.tail, memory_order_acquire)) {
        return NULL;
    }

    // extract the CQE
    index = head & *(iou->cq_ring.ring_mask);
    cqe_extracted = &iou->cq_ring.cqes[index];

    // save the CQE info
    memcpy(cqe_out, cqe_extracted, sizeof(*cqe_extracted));

    // consume the CQE and announce it
    ++head;
    atomic_store_explicit(iou->cq_ring.head, head, memory_order_release);

    return cqe_out;
}


/* =========================================================================
 *  Definitions for IO Uring debugging
 * ========================================================================= */

void io_uring_debug_print_rings(const struct iou * const iou)
{
    unsigned int sq_head, sq_tail;
    unsigned int cq_head, cq_tail;

    sq_head = atomic_load_explicit(iou->sq_ring.head, memory_order_acquire);
    sq_tail = atomic_load_explicit(iou->sq_ring.tail, memory_order_acquire);

    cq_head = atomic_load_explicit(iou->cq_ring.head, memory_order_acquire);
    cq_tail = atomic_load_explicit(iou->cq_ring.tail, memory_order_acquire);

    dlog(
        LOG_DEBUG,
        "sq: {tail: %u | head: %u | entries: %u}, cq: {tail: %u | head: %u | entries: %u}\n",
        sq_tail, sq_head, iou->configuration.sq_entries, cq_tail, cq_head, iou->configuration.cq_entries
    );
}




/* =========================================================================
 *  Definitions for the memory allocation tracker
 * ========================================================================= */

struct iou_op_mem_tracker mem_t_create(void)
{
    struct iou_op_mem_tracker ret = {
        .head = NULL,
        .tail = NULL
    };

    return ret;
}

void mem_t_push_back(
    struct iou_op_mem_tracker * const tracker,
    struct iou_op_mem_tracker_node * const node
)
{
    if (tracker->head == NULL) {
        tracker->head = tracker->tail = node;
        node->next = node->prev = NULL;
        return;
    }

    node->prev = tracker->tail;
    node->next = NULL;

    tracker->tail->next = node;
    tracker->tail = node;
}

void mem_t_remove_node(
    struct iou_op_mem_tracker * const tracker,
    struct iou_op_mem_tracker_node * const node
)
{
    if (node->next) {
        node->next->prev = node->prev;
    }

    if (node->prev) {
        node->prev->next = node->next;
    }

    // check and repair the head and tail if needed
    if (node == tracker->head) {
        tracker->head = node->next;
    }
    if (node == tracker->tail) {
        tracker->tail = node->prev;
    }

    node->next = node->prev = NULL;
}
