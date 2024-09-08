#ifndef UTIL_H_
#define UTIL_H_ 1

#include <stdlib.h> /* exit() */

#define LENGTH(ARR) ((int)(sizeof(ARR) / sizeof((ARR)[0])))

/* Wrapper for err_msg() */
#define ERR(...) err_msg(__func__, __VA_ARGS__)

/* Set the instruction(s) to be executed when SL_EXPECT() fails */
#define SL_ON_ERR(INSTRUCTIONS) \
    if (0) {                    \
sl_lbl_on_err:                  \
        INSTRUCTIONS;           \
    }

/* If COND is not true, show error and jump to instruction declared by
 * SL_ON_ERR() */
#define SL_EXPECT(COND, ...)                                         \
    do {                                                             \
        if (!(COND)) {                                               \
            ERR(__VA_ARGS__);                                        \
            goto sl_lbl_on_err; /* Make sure you call SL_ON_ERR() */ \
        }                                                            \
    } while (0)

/* Show error message and exit */
#define SL_FATAL(...)                \
    do {                             \
        fprintf(stderr, "[Fatal] "); \
        ERR(__VA_ARGS__);            \
        exit(1);                     \
    } while (0)

/* If COND is not true, show error and exit */
#define SL_ASSERT(COND, ...)       \
    do {                           \
        if (!(COND)) {             \
            SL_FATAL(__VA_ARGS__); \
        }                          \
    } while (0)

/* Avoid -Wunused-parameter */
#define SL_UNUSED(VAR) (void)VAR

#define sl_safe_realloc(PTR, SZ)              \
    do {                                      \
        PTR = realloc(PTR, SZ);               \
        if (PTR == NULL) {                    \
            SL_FATAL("Reallocation failed."); \
        }                                     \
    } while (0)

/*----------------------------------------------------------------------------*/

/* Print error message to stderr, along with the function name */
void err_msg(const char* func, const char* fmt, ...);

/* Allocate `sz' bytes using `malloc' or `calloc', ensuring a valid pointer is
 * returned. */
void* sl_safe_malloc(size_t sz);
void* sl_safe_calloc(size_t nmemb, size_t size);

/* Allocate a new string big enough to hold `s', and copy it. Ensures a valid
 * pointer is returned. */
char* sl_safe_strdup(const char* s);

#endif /* UTIL_H_ */
