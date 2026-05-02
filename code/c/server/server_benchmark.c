#include "./server_benchmark.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "../common/debug.h"
#include "../common/measurements.h"
#include "../common/time_utils.h"
#include "../common/utils.h"


struct server_bench sb_create(const int expected_no_events)
{
    struct server_bench instance;

    memset(&instance, 0, sizeof(instance));

    instance.event_latency = meas_pcontainer_create(
        sizeof(struct meas_value_holder), expected_no_events
    );

    return instance;
}

void sb_free(struct server_bench * const instance)
{
    meas_pcontainer_free(&instance->event_latency);
}

void sb_start(struct server_bench * const this)
{
    if (this->do_measure) {
        return;
    }
    this->do_measure = 1;
    this->meas_start_time = now_monotonic();
}

void sb_stop(struct server_bench * const this)
{
    this->do_measure = 0;
    this->meas_end_time = now_monotonic();
}

void sb_client_connected(struct server_bench * const this)
{
    ++this->client_no;
}

void sb_requests_performed(struct server_bench * const this, const int count)
{
    if (!this->do_measure) {
        return;
    }
    this->request_no += count;
}

void sb_record_event(struct server_bench * const this,
    const struct timespec start, const struct timespec end)
{
    int rc;
    struct timespec delta;
    uint64_t delta_ns;
    meas_value_holder_t value_held;

    if (!this->do_measure) {
        return;
    }

    delta = time_delta(end, start);
    delta_ns = timespec_to_ns(delta);
    value_held = meas_create_value_holder(delta_ns);

    rc = meas_pcontainer_store(
        &this->event_latency,
        &value_held,
        sizeof(value_held)
    );

    if (rc < 0) {
        sb_stop(this);
        dlog(LOG_EMERG, "could not store value, stopping benching");
    }
}

void sb_save_bench(const struct server_bench * const this)
{
    size_t i;
    meas_value_holder_t *holder;
    const char bench_file_name[] = "./bench_file.json";
    FILE *bench_file;

    if (this->do_measure) {
        return; // benchmarking is still running
    }

    bench_file = fopen(bench_file_name, "w");
    assert_nnull(bench_file, "fopen");

    // start writing the json
    fprintf(bench_file, "{");
    fprintf(bench_file, "\"meas_start_time\":%lu,",
        timespec_to_ns(this->meas_start_time));
    fprintf(bench_file, "\"meas_end_time\":%lu,",
        timespec_to_ns(this->meas_end_time));
    fprintf(bench_file, "\"client_no\":%d,", this->client_no);
    fprintf(bench_file, "\"request_no\":%lu,", this->request_no);

    // store the elements of the container
    fprintf(bench_file, "\"event_latency\": [");

    for (i = 0; i < this->event_latency.size; ++i) {
        holder = (meas_value_holder_t *)(
            this->event_latency.ptr + i * this->event_latency.entry_size);

        fprintf(bench_file, "{\"value\":%lu}", holder->value);

        if (i != this->event_latency.size - 1) {
            fprintf(bench_file, ",");
        }
    }

    fprintf(bench_file, "]");
    fprintf(bench_file, "}");

    fclose(bench_file);
}
