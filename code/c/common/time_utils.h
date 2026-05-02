// Note: the user should worry about overflows, especially when using ns
#ifndef CODE_C_COMMON_TIME_UTILS_H_
#define CODE_C_COMMON_TIME_UTILS_H_


#include <stdint.h>
#include <time.h>


#define NUMBER_10E9 1000000000L


/* Time getting */
struct timespec now_monotonic(void);
void now_monotonic_p(struct timespec * const ts);
uint64_t now_monotonic_ns(void);
void now_monotonic_ns_p(uint64_t * const ns);

/* Time manipulation */
uint64_t timespec_to_ns(const struct timespec ts);
struct timespec ns_to_timespec(const uint64_t ns);
struct timespec time_delta(const struct timespec ts_1, const struct timespec ts_2);
/*
 * @brief: Make both s and ns fields have the same sign around 0. Also reduces
 * ns and puts it in s.
*/
void timespec_rebalance(struct timespec * const ts);

/* Time comparison */
int timespec_lt(const struct timespec ts_1, const struct timespec ts_2);


#endif /* CODE_C_COMMON_TIME_UTILS_H_ */
