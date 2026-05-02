#ifndef CODE_C_COMMON_UTILS_H_
#define CODE_C_COMMON_UTILS_H_

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "./debug.h"


#define __assertion_prints_and_exit__(level, format, ...)  \
    do {                                                   \
        dlog(level, format, ##__VA_ARGS__);                \
        dlog(level, "[ERRNO]: %s\n", strerror(errno));     \
        exit(EXIT_FAILURE);                                \
    } while (0)


/* Assert code is zero */
#define assert_zero(code, format, ...)                                      \
    do {                                                                    \
        if (code != 0) {                                                    \
            __assertion_prints_and_exit__(LOG_ERR, format, ##__VA_ARGS__);  \
        }                                                                   \
    } while (0)


/* Assert ptr is not NULL */
#define assert_nnull(ptr, format, ...)                                      \
    do {                                                                    \
        if (ptr == NULL) {                                                  \
            __assertion_prints_and_exit__(LOG_ERR, format, ##__VA_ARGS__);  \
        }                                                                   \
    } while (0)


/* Assert code is nonnegative integer */
#define assert_nonn(code, format, ...)                                      \
    do {                                                                    \
        if (code < 0) {                                                     \
            __assertion_prints_and_exit__(LOG_ERR, format, ##__VA_ARGS__);  \
        }                                                                   \
    } while (0)

#endif /* CODE_C_COMMON_UTILS_H_ */
