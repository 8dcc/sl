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

#include <stdlib.h> /* exit() */

/*
 * Wrapper for `sl_print_err'. Should only be used for errors about the
 * interpreter itself; for Lisp errors, use the `err' function.
 */
#define SL_ERR(...) sl_print_err(__func__, __VA_ARGS__)

/*
 * Handle Lisp errors.
 *
 * TODO: This should be a function that doesn't print anything, but instead
 * returns an EXPR_ERR that gets propagated upwards.
 */
#define err SL_ERR

/*
 * Show error message with `sl_print_ftl' and exit.
 */
#define SL_FATAL(...)                                            \
    do {                                                         \
        sl_print_ftl(__FILE__, __LINE__, __func__, __VA_ARGS__); \
        exit(1);                                                 \
    } while (0)

/*
 * If COND is zero, show error message and exit.
 */
#define SL_ASSERT(COND)                                  \
    do {                                                 \
        if ((COND) == 0) {                               \
            SL_FATAL("Assertion `%s' failed.\n", #COND); \
        }                                                \
    } while (0)

/*
 * Set the instruction(s) to be executed when SL_EXPECT() fails.
 */
#define SL_ON_ERR(INSTRUCTIONS) \
    if (0) {                    \
sl_lbl_on_err:                  \
        INSTRUCTIONS;           \
    }

/*
 * If COND is not true, show warning and jump to instruction declared by
 * SL_ON_ERR().
 *
 * TODO: Instea of using SL_ON_ERR, return the EXPR_ERR returned by `err'.
 */
#define SL_EXPECT(COND, ...)                                         \
    do {                                                             \
        if ((COND) == 0) {                                           \
            err(__VA_ARGS__);                                        \
            goto sl_lbl_on_err; /* Make sure you call SL_ON_ERR() */ \
        }                                                            \
    } while (0)

/*
 * Check if the specified linked list of `Expr' structures has a specific
 * length using `SL_EXPECT'.
 */
#define SL_EXPECT_ARG_NUM(EXPR_LIST, NUM)                    \
    SL_EXPECT(expr_list_len(EXPR_LIST) == (NUM),             \
              "Expected exactly %d arguments, got %d.", NUM, \
              expr_list_len(EXPR_LIST))

/*
 * Check if the specified expression matches the expected type using
 * `SL_EXPECT'.
 */
#define SL_EXPECT_TYPE(EXPR, TYPE)                           \
    SL_EXPECT((EXPR)->type == (TYPE),                        \
              "Expected expression of type '%s', got '%s'.", \
              exprtype2str(TYPE), exprtype2str((EXPR)->type))

/*----------------------------------------------------------------------------*/

/*
 * Print different error messages to stderr, along with some context
 * information.
 */
void sl_print_err(const char* func, const char* fmt, ...);
void sl_print_ftl(const char* file, int line, const char* func, const char* fmt,
                  ...);

#endif /* ERROR_H_ */