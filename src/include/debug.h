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
 * SL. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DEBUG_H_
#define DEBUG_H_ 1

#include <stdbool.h>
#include <stdio.h> /* FILE */

struct Env;  /* env.h */
struct Expr; /* expr.h */

/*----------------------------------------------------------------------------*/

/*
 * Is the specified function expression being traced according to
 * 'g_debug_trace_list'?
 */
bool debug_is_traced_function(const struct Expr* e);

/*
 * Print the opening and closing of a function trace.
 *
 * The 'func' expression should be the un-evaluated function (usually a symbol);
 * the 'args' pointer should be a linked list with the evaluated arguments, or
 * NULL; and the 'applied' argument should be a pointer to the evaluated result
 * of the procedure call.
 */
void debug_trace_print_pre(FILE* fp, const struct Expr* func,
                           const struct Expr* args);
void debug_trace_print_post(FILE* fp, const struct Expr* e);

/*----------------------------------------------------------------------------*/

/*
 * Allocate and initialize the internal callstack. True is returned on success,
 * or false otherwise.
 *
 * The internal callstack will be reallocated if necessary when calling
 * 'debug_callstack_push'.
 *
 * The caller is responsible for calling this function only once (or an
 * assertion will fail).
 */
bool debug_callstack_init(void);

/*
 * Get the current position in the debug callstack.
 */
size_t debug_callstack_get_pos(void);

/*
 * Free the internal callstack.
 *
 * If the callstack has been freed, the function ignores it but does not fail.
 */
void debug_callstack_free(void);

/*
 * Push the specified expression into the callstack.
 */
void debug_callstack_push(const struct Expr* e);

/*
 * Pop the last expression from the callstack.
 */
void debug_callstack_pop(void);

/*
 * Print the callstack to the specified file.
 */
void debug_callstack_print(FILE* fp);

#endif /* DEBUG_H_ */
