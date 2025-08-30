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

#include <stdarg.h>
#include <stdio.h>

#include "include/expr.h"
#include "include/util.h"
#include "include/memory.h"
#include "include/debug.h"
#include "include/error.h"

#define COL_RESET       "\x1B[0m"
#define COL_NORM_YELLOW "\x1B[0;33m"
#define COL_NORM_RED    "\x1B[0;31m"
#define COL_BOLD_CYAN   "\x1B[1;36m"
#define COL_BOLD_RED    "\x1B[1;31m"

Expr* err(const char* fmt, ...) {
    va_list va;

#ifdef SL_CALLSTACK_ON_ERR
    /*
     * The callstack will be printed as soon as the error is created, so it will
     * be shown before the REPL receives and prints the error itself. This
     * doesn't look great, but it's still practical.
     */
    debug_callstack_print(stderr);
#endif /* SL_CALLSTACK_ON_ERR */

    va_start(va, fmt);
    const int data_size = vsnprintf(NULL, 0, fmt, va);
    va_end(va);

    char* result = mem_alloc(data_size + 1);

    va_start(va, fmt);
    vsnprintf(result, data_size + 1, fmt, va);
    va_end(va);

    Expr* ret  = expr_new(EXPR_ERR);
    ret->val.s = result;
    return ret;
}

void err_print(FILE* fp, const Expr* e) {
    SL_ASSERT(e != NULL);
    SL_ASSERT(EXPR_ERR_P(e));
    SL_ASSERT(e->val.s != NULL);

#ifdef SL_NO_COLOR
    fprintf(fp, "Error: %s", e->val.s);
#else  /* not SL_NO_COLOR) */
    fprintf(fp,
            "%sError%s: %s%s%s",
            COL_BOLD_RED,
            COL_RESET,
            COL_NORM_YELLOW,
            e->val.s,
            COL_RESET);
#endif /* not SL_NO_COLOR */
}

/*----------------------------------------------------------------------------*/

void sl_print_err(const char* func, const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);

#ifdef SL_NO_COLOR
    fprintf(stderr, "%s: ", func);
    vfprintf(stderr, fmt, va);
#else  /* not SL_NO_COLOR */
    fprintf(stderr,
            "%s%s%s: %s",
            COL_BOLD_CYAN,
            func,
            COL_RESET,
            COL_NORM_YELLOW);
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "%s", COL_RESET);
#endif /* not SL_NO_COLOR */

    fputc('\n', stderr);

    va_end(va);
}

void sl_print_ftl(const char* file,
                  int line,
                  const char* func,
                  const char* fmt,
                  ...) {
    va_list va;
    va_start(va, fmt);

#ifdef SL_NO_COLOR
    fprintf(stderr, "%s:%d: %s: ", file, line, func);
    vfprintf(stderr, fmt, va);
#else  /* not SL_NO_COLOR */
    fprintf(stderr,
            "%s:%d: %s%s%s: %s",
            file,
            line,
            COL_BOLD_CYAN,
            func,
            COL_RESET,
            COL_NORM_RED);
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "%s", COL_RESET);
#endif /* not SL_NO_COLOR */

    fputc('\n', stderr);

    va_end(va);
}
