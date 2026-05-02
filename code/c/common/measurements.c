#include "./measurements.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "./utils.h"


meas_value_holder_t meas_create_value_holder(const uint64_t value)
{
    meas_value_holder_t holder = {
        .value = value,
    };
    return holder;
}

meas_pcontainer_t meas_pcontainer_create(const size_t entry_size, const size_t capacity)
{
    meas_pcontainer_t container = {
        .entry_size = entry_size,
        .capacity = capacity,
        .size = 0U,
        .ptr = NULL,
    };

    container.ptr = (void*)malloc(entry_size * capacity);
    assert_nnull(container.ptr, "malloc");

    return container;
}

int meas_pcontainer_store(meas_pcontainer_t * const container,
    const void * const holder, const size_t holder_size)
{
    if (container->entry_size != holder_size) {
        return -1;
    }
    if (container->size == container->capacity) {
        return -1;
    }

    memcpy(container->ptr + container->size * holder_size, holder, holder_size);
    container->size += 1;

    return 0;
}

void meas_pcontainer_free(meas_pcontainer_t * const container)
{
    free(container->ptr);
    container->ptr = NULL;
    container->size = 0;
}
