#ifndef CODE_C_COMMON_MEASUREMENTS_H_
#define CODE_C_COMMON_MEASUREMENTS_H_


#include <stdint.h>
#include <time.h>

#include "./utils.h"
#include "./time_utils.h"


/* Holds the value and adds the monotonic time to its info */
typedef struct meas_value_holder {
    uint64_t value;
    // struct timespec ts;
} meas_value_holder_t;

/* Preallocated container to hold anything */
typedef struct meas_pcontainer {
    size_t entry_size;
    size_t capacity;
    size_t size;
    void *ptr;
} meas_pcontainer_t;


meas_value_holder_t meas_create_value_holder(const uint64_t value);

meas_pcontainer_t meas_pcontainer_create(const size_t entry_size,
    const size_t capacity);
int meas_pcontainer_store(meas_pcontainer_t * const container,
    const void * const holder, const size_t holder_size);
void meas_pcontainer_free(meas_pcontainer_t * const container);


#endif /* CODE_C_COMMON_MEASUREMENTS_H_ */
