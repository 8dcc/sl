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
#include "include/debug.h"

static size_t trace_nesting = 0;

static void print_trace_number(FILE* fp) {
    for (size_t i = 0; i <= trace_nesting; i++)
        fprintf(fp, "  ");

    fprintf(fp, "%zu: ", trace_nesting % 10);
}

bool debug_is_traced_function(const Env* env, const Expr* e) {
    /*
     * TODO: env_get() is being called way too many times here, specially
     * considering the symbol is in the top-most environment. We should check
     * from a C list, and somehow allow the user to add items to it.
     */
    Expr* debug_trace_list = env_get(env, "*debug-trace*");
    if (debug_trace_list == NULL || debug_trace_list->type != EXPR_PARENT)
        return false;

    return expr_list_is_member(debug_trace_list->val.children, e);
}

void debug_trace_print_pre(FILE* fp, const Expr* func, const Expr* arg) {
    print_trace_number(fp);

    fputc('(', fp);
    expr_print(fp, func);
    for (; arg != NULL; arg = arg->next) {
        fputc(' ', fp);
        expr_print(fp, arg);
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
        expr_print(fp, e);

    putchar('\n');
}
