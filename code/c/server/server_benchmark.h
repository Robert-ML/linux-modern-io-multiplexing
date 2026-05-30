#ifndef CODE_C_SERVER_SERVER_BENCHMARK_H_
#define CODE_C_SERVER_SERVER_BENCHMARK_H_

#include <stdint.h>
#include <time.h>

#include "../common/measurements.h"


#ifndef DO_SERVER_SIDE_BENCHMARKING
#define DO_SERVER_SIDE_BENCHMARKING 0
#endif

#ifndef BENCH_MEASURE_SERVICING_LATENCY
#define BENCH_MEASURE_SERVICING_LATENCY 1
#endif


/**
 * Struct to hold the benchmarking info of the server. It holds:
 * - the number of requests performed;
 * - the start and end time of the benchmarking;
 * - container to hold the latency of the events (stored in ns).
 */
typedef struct server_bench {
    int do_measure;
    int client_no;
    uint64_t request_no;

    // measuring start time
    struct timespec meas_start_time;
    // measuring end time
    struct timespec meas_end_time;

    meas_pcontainer_t event_latency;
} server_bench_t;


struct server_bench sb_create(const int expected_no_events);
void sb_free(struct server_bench * const instance);
void sb_start(struct server_bench * const this);
void sb_stop(struct server_bench * const this);
void sb_client_connected(struct server_bench * const this);

// These methods' functioning depends on the state of the field `do_measure`
void sb_requests_performed(struct server_bench * const this, const int count);
void sb_record_event(struct server_bench * const this,
    const struct timespec start, const struct timespec end);
void sb_save_bench(const struct server_bench * const this);

#endif /* CODE_C_SERVER_SERVER_BENCHMARK_H_ */
