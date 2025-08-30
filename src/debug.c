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

#include <stdbool.h>
#include <stdio.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/memory.h"
#include "include/debug.h"

/*
 * Function used for printing expressions in 'debug_*' functions.
 */
#define DEBUG_EXPR_PRINT expr_print

#define DEBUG_CALLSTACK_BASE_SZ 100

/*----------------------------------------------------------------------------*/

/*
 * Number of nested functions that we are currently tracing.
 */
static size_t trace_nesting = 0;

/*
 * The callstack pointer (an array of 'Expr' pointers), its size and the current
 * position.
 */
static const Expr** callstack = NULL;
static size_t callstack_sz    = 0;
static size_t callstack_pos   = 0;

/*----------------------------------------------------------------------------*/

static void print_trace_number(FILE* fp) {
    for (size_t i = 0; i <= trace_nesting; i++)
        fprintf(fp, "  ");

    fprintf(fp, "%zu: ", trace_nesting % 10);
}

bool debug_is_traced_function(const Expr* e) {
    const Expr* trace_list = g_debug_trace_list;
    SL_ASSERT(trace_list != NULL);

    if (expr_is_nil(trace_list))
        return false;

    return expr_is_member(e, trace_list);
}

void debug_trace_print_pre(FILE* fp, const Expr* func, const Expr* arg) {
    SL_ASSERT(expr_is_proper_list(arg));

    print_trace_number(fp);

    fputc('(', fp);
    DEBUG_EXPR_PRINT(fp, func);
    for (; !expr_is_nil(arg); arg = CDR(arg)) {
        fputc(' ', fp);
        DEBUG_EXPR_PRINT(fp, CAR(arg));
    }
    fprintf(fp, ")\n");

    trace_nesting++;
}

void debug_trace_print_post(FILE* fp, const Expr* e) {
    trace_nesting--;
    print_trace_number(fp);

    if (e == NULL)
        fprintf(fp, "ERR");
    else
        DEBUG_EXPR_PRINT(fp, e);

    putchar('\n');
}

/*----------------------------------------------------------------------------*/

bool debug_callstack_init(void) {
    SL_ASSERT(callstack == NULL);
    callstack = mem_calloc(DEBUG_CALLSTACK_BASE_SZ, sizeof(Expr*));
    return true;
}

size_t debug_callstack_get_pos(void) {
    SL_ASSERT(callstack != NULL);
    return callstack_pos;
}

void debug_callstack_free(void) {
    if (callstack == NULL)
        return;

    mem_free(callstack);
    callstack = NULL;
}

void debug_callstack_push(const Expr* e) {
    if (callstack_pos >= callstack_sz) {
        callstack_sz += DEBUG_CALLSTACK_BASE_SZ;
        mem_realloc(&callstack, callstack_sz * sizeof(Expr*));
    }

    callstack[callstack_pos++] = e;
}

void debug_callstack_pop(void) {
    SL_ASSERT(callstack_pos > 0);
    callstack[--callstack_pos] = NULL;
}

void debug_callstack_print(FILE* fp) {
    if (callstack_pos == 0) {
        fprintf(fp, "Callstack: (no callstack)\n");
        return;
    }

    fprintf(fp, "Callstack (recent first):\n");
    for (size_t i = callstack_pos, j = 0; i > 0; i--, j++) {
        const size_t real_i = i - 1;
        SL_ASSERT(callstack[real_i] != NULL);
        fprintf(fp, "  %zu: ", j);
        DEBUG_EXPR_PRINT(fp, callstack[real_i]);
        putchar('\n');
    }
}
