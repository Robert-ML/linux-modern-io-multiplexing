#ifndef CODE_C_SERVER_POLL_POLLFD_VECTOR_H_
#define CODE_C_SERVER_POLL_POLLFD_VECTOR_H_


#include <stdint.h>
#include <sys/poll.h>


typedef struct pollfd_vector {
    struct pollfd *data;
    uint32_t size;
    uint32_t capacity;
    void ** pd; // private data
} pollfd_vector_t;


struct pollfd_vector pollfdv_create(void);
void  pollfdv_free(struct pollfd_vector * const pollfdv);
void pollfdv_add(
    struct pollfd_vector * const pollfdv,
    const struct pollfd conf,
    void * private_data
);
/**
 * @brief: Removes the element at the given index. If iterating the vector,
 * this operation invalidates the iteration as it reorganizes elements. The
 * removed index has now what used to be the last element in the vector.
 */
void pollfdv_remove_index(
    struct pollfd_vector * const pollfdv,
    const uint32_t index
);


#endif /* CODE_C_SERVER_POLL_POLLFD_VECTOR_H_ */
