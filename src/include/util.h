#ifndef UTIL_H_
#define UTIL_H_ 1

#include <stdbool.h>
#include <stdio.h>  /* fprintf() */
#include <stdlib.h> /* exit() */
#include <regex.h>  /* regmatch_t */

/*----------------------------------------------------------------------------*/

/* See strtoll(3) */
#define STRTOLL_ANY_BASE 0

/*----------------------------------------------------------------------------*/

#define LENGTH(ARR) ((int)(sizeof(ARR) / sizeof((ARR)[0])))

#define MIN(A, B)        ((A) < (B) ? (A) : (B))
#define MAX(A, B)        ((A) > (B) ? (A) : (B))
#define CLAMP(N, LO, HI) (MIN(MAX((LO), (N)), (HI)))

/*
 * Wrapper for err_msg().
 */
#define ERR(...) err_msg(__func__, __VA_ARGS__)

/*
 * Set the instruction(s) to be executed when SL_EXPECT() fails.
 */
#define SL_ON_ERR(INSTRUCTIONS) \
    if (0) {                    \
sl_lbl_on_err:                  \
        INSTRUCTIONS;           \
    }

/*
 * If COND is not true, show error and jump to instruction declared by
 * SL_ON_ERR().
 */
#define SL_EXPECT(COND, ...)                                         \
    do {                                                             \
        if (!(COND)) {                                               \
            ERR(__VA_ARGS__);                                        \
            goto sl_lbl_on_err; /* Make sure you call SL_ON_ERR() */ \
        }                                                            \
    } while (0)

/*
 * Show error message and exit.
 */
#define SL_FATAL(...)                \
    do {                             \
        fprintf(stderr, "[Fatal] "); \
        ERR(__VA_ARGS__);            \
        exit(1);                     \
    } while (0)

/*
 * If COND is not true, show error and exit.
 */
#define SL_ASSERT(COND, ...)       \
    do {                           \
        if (!(COND)) {             \
            SL_FATAL(__VA_ARGS__); \
        }                          \
    } while (0)

/*
 * Avoid -Wunused-parameter
 */
#define SL_UNUSED(VAR) (void)VAR

/*
 * Use a macro to avoid assignment.
 */
#define sl_safe_realloc(PTR, SZ)              \
    do {                                      \
        PTR = realloc(PTR, SZ);               \
        if (PTR == NULL) {                    \
            SL_FATAL("Reallocation failed."); \
        }                                     \
    } while (0)

/*----------------------------------------------------------------------------*/

/*
 * Print error message to stderr, along with the function name.
 */
void err_msg(const char* func, const char* fmt, ...);

/*
 * Allocate `sz' bytes using `malloc' or `calloc', ensuring a valid pointer is
 * returned.
 */
void* sl_safe_malloc(size_t sz);
void* sl_safe_calloc(size_t nmemb, size_t size);

/*
 * Allocate a new string big enough to hold `s', and copy it. Ensures a valid
 * pointer is returned.
 */
char* sl_safe_strdup(const char* s);

/*
 * Return the actual value of an escape sequence character. If the character is
 * not part of a supported escape sequence, an error message is printed to
 * `stderr' and the character is returned unchanged.
 *
 * For example, 'n' -> 0xA (\n).
 */
char escaped2byte(char escaped);

/*
 * Return a two-character escape sequence that represents the specified byte. If
 * the input is not the value of a supported escape sequence, NULL is returned.
 *
 * For example, 0xA -> "\n" (that is, '\\' and 'n').
 */
const char* byte2escaped(char byte);

/*
 * Print a string with values corresponding to escape sequences. The printed
 * string should evaluate to the input.
 */
void print_escaped_str(const char* s);

/*
 * Find all matches of `pat' in `str', writing the number of sub-expression
 * matches in `nmatch' and writing an array of `nmatch + 1' elements in
 * `pmatch'. See regexec(3) for more information.
 *
 * The function returns true if the pattern compilation succedeed and there was
 * a match. If (and only if) true is returned, the caller is responsible for
 * freeing `pmatch'.
 */
bool sl_regex_matches(const char* pat, const char* str, bool ignore_case,
                      size_t* nmatch, regmatch_t** pmatch);

#endif /* UTIL_H_ */
