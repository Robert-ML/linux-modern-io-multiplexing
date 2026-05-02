#include "./time_utils.h"

#include <stdint.h>
#include <time.h>


/* ================================================================
 *  Time getting
 * ================================================================ */
struct timespec now_monotonic(void)
{
    struct timespec ts;
    now_monotonic_p(&ts);
    return ts;
}

void now_monotonic_p(struct timespec * const ts)
{
    clock_gettime(CLOCK_MONOTONIC, ts);
}

uint64_t now_monotonic_ns(void)
{
    const struct timespec ts = now_monotonic();
    return timespec_to_ns(ts);
}

void now_monotonic_ns_p(uint64_t * const ns)
{
    const struct timespec ts = now_monotonic();
    *ns = timespec_to_ns(ts);
}


/* ================================================================
 *  Time manipulation
 * ================================================================ */
uint64_t timespec_to_ns(const struct timespec ts)
{
    return (uint64_t)ts.tv_sec * NUMBER_10E9 + ts.tv_nsec;
}

struct timespec ns_to_timespec(const uint64_t ns)
{
    struct timespec ts;
    ts.tv_sec = ns / NUMBER_10E9;
    ts.tv_nsec = ns % NUMBER_10E9;
    return ts;
}

/*
 * @brief: Calculates the delta of 2 timespecs.
 *
 * @returns: A balanced (see `timespec_rebalance`) new timespec structure.
 *
 * @note: The passed arguments are expected to be balanced to reduce the
 * possibility of overflows.
 */
struct timespec time_delta(const struct timespec minuend, const struct timespec subtrahend)
{
    struct timespec delta;

    // 1 200 - 1 300 = 0 -100
    delta.tv_sec = minuend.tv_sec - subtrahend.tv_sec;
    delta.tv_nsec = minuend.tv_nsec - subtrahend.tv_nsec;

    timespec_rebalance(&delta);

    return delta;
}

/*
 * @brief: Balances a timespec structure in place. I.e. hole seconds are
 * extracted from the nsec field, and the sec and nsec fields will have the
 * same sign or one will be 0.
 *
 * Example:
 * {sec:  2, nsec: -13e5} => {sec:  0, nsec: nsec:  7e5} (balanced)
 * {sec:  2, nsec:  -3e5} => {sec:  1, nsec: nsec:  7e5} (balanced)
 * {sec: -2, nsec: -13e5} => {sec: -3, nsec: nsec: -3e5} (balanced)
 * {sec: -2, nsec:  13e5} => {sec:  0, nsec: nsec: -7e5} (balanced)
 */
void timespec_rebalance(struct timespec * const ts)
{
    //  2 -1300 =>  1 -300 =>  0  700
    // -4  1300 => -3  300 => -2 -700
    // -1  1300 =>  0  300 =>  0  300
    //  1 -1300 =>  0 -300 =>  0 -300
    // -2  1000 => -1    0 => -1    0
    //  2 -1000 =>  1    0 =>  1    0
    int hole_parts = ts->tv_nsec / NUMBER_10E9;
    ts->tv_sec += hole_parts;
    ts->tv_nsec = ts->tv_nsec % NUMBER_10E9;

    if (ts->tv_sec == 0 || ts->tv_nsec == 0) {
        return;
    }

    int sec_negative = (ts->tv_sec < 0);
    int nsec_negative = (ts->tv_nsec < 0);

    if (sec_negative == nsec_negative) {
        return;
    }

    if (nsec_negative) {
        // sec > 0 && nsec < 0
        ts->tv_nsec += NUMBER_10E9;
        ts->tv_sec -= 1;
    } else {
        // sec < 0 && nsec > 0
        ts->tv_nsec -= NUMBER_10E9;
        ts->tv_sec += 1;
    }
}

/* ================================================================
 *  Time comparison
 * ================================================================ */
/**
 * @brief: Compares if `ts_1` < `ts_2`. The arguments must be balanced (see
 * `timespec_rebalance`).
 */
int timespec_lt(const struct timespec ts_1, const struct timespec ts_2)
{
    if (ts_1.tv_sec == ts_2.tv_sec) {
        return ts_1.tv_nsec < ts_2.tv_nsec;
    } else {
        return ts_1.tv_sec < ts_2.tv_sec;
    }
}
