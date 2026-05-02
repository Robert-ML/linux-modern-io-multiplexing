#include <stdio.h>
#include <unistd.h>

#include "../server_benchmark.h"
#include "../server_utils.h"
#include "../../common/debug.h"
#include "../../common/defines.h"
#include "../../common/measurements.h"
#include "../../common/time_utils.h"
#include "../../common/utils.h"


typedef struct pd_echo_connection {
    int fd;
    uint32_t buf_size;
    uint8_t buf[DEFAULT_SERVER_BUFFER_SIZE];
} pd_echo_connection_t;


/* ================================================================
 *  Benchmarking variables
 * ================================================================ */
struct server_bench bench;


int main(int, char **)
{
    int listening_socket;

    bench = sb_create(BENCH_EXPECTED_MESSAGES);
    register_signal_handler();

    printf("Poll Echo Server (PID: %d)\n--- Starting!\n\n", getpid());

    return 0;
}