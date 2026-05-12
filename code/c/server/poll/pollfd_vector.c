#include "./pollfd_vector.h"


#include <stdint.h>
#include <stdlib.h>

#include "../../common/debug.h"
#include "../../common/utils.h"


/* =========================================================================
 *  Defines
 * ========================================================================= */
#define DATA_INITIAL_SIZE 16
#define DATA_CAPACITY_EXTEND_MULT 2


/* =========================================================================
 *  Static function declarations
 * ========================================================================= */

static void pollfdv_extend(struct pollfd_vector * const pollfdv);


/* =========================================================================
 *  Function definitions
 * ========================================================================= */

struct pollfd_vector pollfdv_create(void)
{
    uint32_t i;
    pollfd_vector_t pollfdv;

    pollfdv.size = 0;
    pollfdv.capacity = DATA_INITIAL_SIZE;
    pollfdv.data = (struct pollfd *)calloc(
        pollfdv.capacity, sizeof(pollfdv.data[0])
    );
    assert_nnull(pollfdv.data, "calloc pollfdv.data");
    pollfdv.pd = calloc(pollfdv.capacity, sizeof(pollfdv.pd[0]));
    assert_nnull(pollfdv.pd, "calloc pollfdv.pd");

    for (i = 0; i < pollfdv.capacity; ++i) {
        pollfdv.data[i].fd = -1;
    }

    return pollfdv;
}

void pollfdv_free(struct pollfd_vector * const pollfdv)
{
    free(pollfdv->data);
    free(pollfdv->pd);
    pollfdv->size = 0;
    pollfdv->capacity = 0;
}

void pollfdv_add(
    struct pollfd_vector * const pollfdv,
    const struct pollfd conf,
    void * private_data
)
{
    if (pollfdv->size >= pollfdv->capacity) {
        pollfdv_extend(pollfdv);
    }

    if (pollfdv->size >= pollfdv->capacity) {
        dlog(LOG_CRIT, "Did not find a place to add a new entry in pollfdv");
        exit(EXIT_FAILURE);
    }

    memcpy(&pollfdv->data[pollfdv->size], &conf, sizeof(conf));
    pollfdv->pd[pollfdv->size] = private_data;
    ++pollfdv->size;

}

void pollfdv_remove_index(
    struct pollfd_vector * const pollfdv,
    const uint32_t index
)
{
    if (index >= pollfdv->capacity) {
        dlog(LOG_ALERT, "Tried to remove at index outside capacity");
        return;
    }

    if (pollfdv->data[index].fd == -1) {
        return;
    }

    if (index != pollfdv->size - 1) {
        // replace the removed entry with the last one
        memcpy(
            &pollfdv->data[index],
            &pollfdv->data[pollfdv->size - 1],
            sizeof(pollfdv->data[0])
        );
        pollfdv->pd[index] = pollfdv->pd[pollfdv->size - 1];
    }

    // remove the last entry
    memset(&pollfdv->data[pollfdv->size - 1], 0, sizeof(pollfdv->data[0]));
    pollfdv->pd[pollfdv->size - 1] = NULL;
    pollfdv->data[pollfdv->size - 1].fd = -1;

    --pollfdv->size;
}


static void pollfdv_extend(struct pollfd_vector * const pollfdv)
{
    uint32_t new_capacity = pollfdv->capacity * DATA_CAPACITY_EXTEND_MULT;
    struct pollfd *new_data_ptr;
    void **new_pd_ptr;

    new_data_ptr = (struct pollfd *)realloc(
        pollfdv->data, new_capacity * sizeof(pollfdv->data[0])
    );
    assert_nnull(new_data_ptr, "realloc pollfdv.data");

    new_pd_ptr = realloc(
        pollfdv->pd, new_capacity * sizeof(pollfdv->pd[0])
    );
    assert_nnull(new_pd_ptr, "realloc pollfdv.pd");

    pollfdv->data = new_data_ptr;
    pollfdv->pd = new_pd_ptr;
    pollfdv->capacity = new_capacity;
}
