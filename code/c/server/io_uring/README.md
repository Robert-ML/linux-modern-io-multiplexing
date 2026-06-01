# IO Uring Echo Server

The server runs in 2 modes: simple and `SQPOLL`.


## Discussion on memory barriers needed by io_uring

This is more a note to my future self, or any other person who came across this and will be glad of the explanation about the necessity of the memory barrier. It is important to specify this code is functional only when a single thread is submitting to the SQ_ring.

Let's consider the following pice of code:

```C
void io_uring_submit(struct iou * const iou)
{
     struct io_uring_sqe *sqe;
     unsigned int tail, index;

     // comment label: acquire_sqe
     // extract an available SQE from the ring
     tail = *iou->sq_ring.tail;
     index = tail & (*iou->sq_ring.ring_mask);
     sqe = &iou->sqes[index];

     // configure whatever operation
     // comment label: populate_sqe
     sqe->fd = 5;

     // add the configured SQE in the ring

     // comment label: submit_sqe
     iou->sq_ring.array[index] = index;
     // comment label: announce_new_sqe
     ++tail;
     atomic_store_explicit(iou->sq_ring.tail, tail, memory_order_release);
}
```

At comment label `acquire_sqe`: We have obtained an SQE available for configuration and then submission to the SQ_ring. (Here is important the mention that only one thread submits to the SQ_ring, i.e. there is only one producer for the SQ_ring, i.e. only one thread acquires SQEs at a time).

At comment label `populate_sqe`: We have configured the SQE with whatever operation we wanted.

At comment label `submit_sqe`: We have added the configured SQE (or it's index better said) in the SQ_ring. We can say we submitted the entry. BUT we did not inform the ring yet that it has a new SQE configures, i.e. we did not update the tail of the SQ_ring yet!

At comment label `announce_new_sqe`: Here is the point where we actually let the SQ_ring know it has something new. And this is where we need to be careful of memory and operation reordering by the compiler or the CPU itself. What could happen is that we write:
```C
sqe = &sqes[index]; // acquiring
sqe->fd = 5; // example of config
sq_ring.array[index] = index; // submission
*sq_ring.tail = *sq_ring.tail + 1; // announcement
```

But the CPU and the compiler know only about our executable's universe, and do not know that other threads have access to the mapped memory shared with the kernel. So they will try to optimize by reordering operations in a way they think is more efficient. So there are 2 actors that will play around with our code and the final execution might end up looking like this:
```C
sqe = &sqes[index]; // acquiring
*sq_ring.tail = *sq_ring.tail + 1; // announcement
sqe->fd = 5; // example of config
sq_ring.array[index] = index; // submission
```

Completely another order, but a valid one if the memory is only worked on by a single process. But we are not, and what could happen is:
```C
sqe = &sqes[index]; // acquiring
*sq_ring.tail = *sq_ring.tail + 1; // announcement
// < PROCESS IS PREEMPTED BY THE SCHEDULER >
sqe->fd = 5; // example of config
sq_ring.array[index] = index; // submission
```

If the compiler reordered operations, the process can get preempted right after it updated the tail, but it still did not finish configuring the SQE. While the process is not on the CPU scheduled, there can be a kernel thread consuming the SQ_ring and see there is a new entry (the tail was updated), and then it tries to process the SQE that is still incomplete. Big uff. That is why we force the compiler to not reorder and make sure all the previous operations are done before the writing of the tail.

Now, about the CPU reordering operations, the same kernel thread might look at the tail value right after it was changed and still not update the SQE values. But the kernel thread has seen the updated tail, took a decision based on this and started to process the unconfigured yet SQE.

### Notes

- This is a bigger issue when we configure `io_uring` in `SQPOLL` mode.
- The problem about CPU reordering operations is just a guess. I could not find something explicit to back it up. I suppose the preemption is not an issue for the CPU reordering, but the kernel looking right at the wrong moment.
