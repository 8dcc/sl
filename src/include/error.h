/*
 * Copyright 2024 8dcc
 *
 * This file is part of SL.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ERROR_H_
#define ERROR_H_ 1

#include <stdio.h>  /* FILE */
#include <stdlib.h> /* exit() */

struct Expr; /* expr.h */

/*----------------------------------------------------------------------------*/

/*
 * Wrapper for 'sl_print_err'. Should only be used for errors about the
 * interpreter itself; for Lisp errors, use the 'err' function.
 */
#define SL_ERR(...) sl_print_err(__func__, __VA_ARGS__)

/*
 * Show error message with 'sl_print_ftl' and exit.
 */
#define SL_FATAL(...)                                                          \
    do {                                                                       \
        sl_print_ftl(__FILE__, __LINE__, __func__, __VA_ARGS__);               \
        exit(1);                                                               \
    } while (0)

/*
 * If COND is zero at run-time, show error message and exit.
 */
#define SL_ASSERT(COND)                                                        \
    do {                                                                       \
        if ((COND) == 0) {                                                     \
            SL_FATAL("Assertion `%s' failed.", #COND);                         \
        }                                                                      \
    } while (0)

/*
 * If COND is zero at compile-time, stop.
 */
#define SL_STATIC_ASSERT(COND)                                                 \
    _Static_assert(COND, "Assertion `" #COND "' failed.")

/*
 * Assert that TYPEA is effectively equal to TYPEB.
 */
#define SL_ASSERT_TYPES(TYPEA, TYPEB)                                          \
    SL_STATIC_ASSERT(__builtin_types_compatible_p(TYPEA, TYPEB))

/*----------------------------------------------------------------------------*/

/*
 * If COND is not true, return expression of type EXPR_ERR with the specified
 * message.
 */
#define SL_EXPECT(COND, ...)                                                   \
    do {                                                                       \
        if ((COND) == 0) {                                                     \
            return err(__VA_ARGS__);                                           \
        }                                                                      \
    } while (0)

/*
 * Check if the specified proper list has a specific length using
 * 'expr_list_len'.
 *
 * TODO: Don't call 'expr_list_len' twice.
 */
#define SL_EXPECT_LEN(EXPR_LIST, NUM)                                          \
    SL_EXPECT(expr_list_len(EXPR_LIST) == (NUM),                               \
              "Expected a list of length %d, got %zu.",                        \
              NUM,                                                             \
              expr_list_len(EXPR_LIST))

/*
 * Check if the specified argument list has a specific length using
 * 'expr_list_len'. Similar to 'SL_EXPECT_LEN', but with a different error
 * message.
 */
#define SL_EXPECT_ARG_NUM(EXPR_LIST, NUM)                                      \
    SL_EXPECT(expr_list_len(EXPR_LIST) == (NUM),                               \
              "Expected exactly %d arguments, got %zu.",                       \
              NUM,                                                             \
              expr_list_len(EXPR_LIST))

/*
 * Check if the specified argument list has a minimum length using
 * 'expr_list_len'. Similar to 'SL_EXPECT_ARG_NUM'.
 */
#define SL_EXPECT_MIN_ARG_NUM(EXPR_LIST, NUM)                                  \
    SL_EXPECT(expr_list_len(EXPR_LIST) >= (NUM),                               \
              "Expected at least %d arguments, got %zu.",                      \
              NUM,                                                             \
              expr_list_len(EXPR_LIST))

/*
 * Check if the specified expression matches the specified type.
 */
#define SL_EXPECT_TYPE(EXPR, TYPE)                                             \
    SL_EXPECT((EXPR)->type == (TYPE),                                          \
              "Expected expression of type '%s', got '%s'.",                   \
              exprtype2str(TYPE),                                              \
              exprtype2str((EXPR)->type))

/*
 * Check if the specified expression is a proper list using
 * 'expr_is_proper_list'.
 */
#define SL_EXPECT_PROPER_LIST(EXPR)                                            \
    SL_EXPECT(expr_is_proper_list(EXPR),                                       \
              "Expected a proper list, got '%s'.",                             \
              exprtype2str((EXPR)->type))

/*----------------------------------------------------------------------------*/

/*
 * Create a new error expression with the specified message. This function
 * doesn't directly print anything; the error is supposed to get propagated
 * upwards.
 */
struct Expr* err(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

/*
 * Print an expression of type 'EXPR_ERR' into the specified file. Doesn't print
 * a final newline.
 *
 * Will use colors unless 'SL_NO_COLOR' is defined.
 */
void err_print(FILE* fp, const struct Expr* e);

/*
 * Print different error messages to 'stderr', along with some context
 * information. Prints a final newline.
 *
 * Will use colors unless 'SL_NO_COLOR' is defined.
 */
void sl_print_err(const char* func, const char* fmt, ...)
  __attribute__((format(printf, 2, 3)));
void sl_print_ftl(const char* file, int line, const char* func, const char* fmt,
                  ...) __attribute__((format(printf, 4, 5)));

#endif /* ERROR_H_ */
