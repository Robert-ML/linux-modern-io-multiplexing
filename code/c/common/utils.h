#ifndef CODE_C_COMMON_UTILS_H_
#define CODE_C_COMMON_UTILS_H_

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "./debug.h"


#define __assertion_prints_and_exit__(level, format, given_errno, ...)  \
    do {                                                                \
        dlog(level, format, ##__VA_ARGS__);                             \
        dlog(level, "[ERRNO]: %s\n", strerror(given_errno));            \
        exit(EXIT_FAILURE);                                             \
    } while (0)


/* Assert code is zero */
#define assert_zero(code, format, ...)                                             \
    do {                                                                           \
        if ((code) != 0) {                                                         \
            __assertion_prints_and_exit__(LOG_ERR, format, errno, ##__VA_ARGS__);  \
        }                                                                          \
    } while (0)

/* Assert code is zero or error code */
#define io_uring_assert_zero(code, format, ...)                                      \
    do {                                                                             \
        if ((code) != 0) {                                                           \
            __assertion_prints_and_exit__(LOG_ERR, format, -(code), ##__VA_ARGS__);  \
        }                                                                            \
    } while (0)


/* Assert ptr is not NULL */
#define assert_nnull(ptr, format, ...)                                             \
    do {                                                                           \
        if ((ptr) == NULL) {                                                       \
            __assertion_prints_and_exit__(LOG_ERR, format, errno, ##__VA_ARGS__);  \
        }                                                                          \
    } while (0)


/* Assert code is nonnegative integer */
#define assert_nonn(code, format, ...)                                             \
    do {                                                                           \
        if ((code) < 0) {                                                          \
            __assertion_prints_and_exit__(LOG_ERR, format, errno, ##__VA_ARGS__);  \
        }                                                                          \
    } while (0)

/* Assert code is nonnegative integer */
#define io_uring_assert_nonn(code, format, ...)                                      \
    do {                                                                             \
        if ((code) < 0) {                                                            \
            __assertion_prints_and_exit__(LOG_ERR, format, -(code), ##__VA_ARGS__);  \
        }                                                                            \
    } while (0)


/* Assert `condition` is true or log with `error_code` */
#define io_uring_assert(condition, error_code, format, ...)                                \
    do {                                                                                   \
        if (!(condition)) {                                                                \
            __assertion_prints_and_exit__(LOG_ERR, format, -(error_code), ##__VA_ARGS__);  \
        }                                                                                  \
    } while (0)


#endif /* CODE_C_COMMON_UTILS_H_ */
