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

#include "include/error.h"

#define COL_RESET       "\e[0m"
#define COL_NORM_YELLOW "\e[0;33m"
#define COL_NORM_RED    "\e[0;31m"
#define COL_BOLD_CYAN   "\e[1;36m"
#define COL_BOLD_RED    "\e[1;31m"

void sl_print_err(const char* func, const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);

#ifdef SL_NO_COLOR
    fprintf(stderr, "%s: ", func);
    vfprintf(stderr, fmt, va);
#else
    fprintf(stderr, "%s%s%s: %s", COL_BOLD_CYAN, func, COL_RESET,
            COL_NORM_YELLOW);
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "%s", COL_RESET);
#endif

    fputc('\n', stderr);

    va_end(va);
}

void sl_print_ftl(const char* file, int line, const char* func, const char* fmt,
                  ...) {
    va_list va;
    va_start(va, fmt);

#ifdef SL_NO_COLOR
    fprintf(stderr, "%s:%d: %s: ", file, line, func);
    vfprintf(stderr, fmt, va);
#else
    fprintf(stderr, "%s:%d: %s%s%s: %s", file, line, COL_BOLD_CYAN, func,
            COL_RESET, COL_NORM_RED);
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "%s", COL_RESET);
#endif

    fputc('\n', stderr);

    va_end(va);
}
