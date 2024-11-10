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

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>

#include "include/util.h"
#include "include/memory.h"
#include "include/error.h"

/* clang-format off */
char escaped2byte(char escaped) {
    switch (escaped) {
        case 'a':  return '\a';
        case 'b':  return '\b';
        case 'e':  return '\e';
        case 'f':  return '\f';
        case 'n':  return '\n';
        case 'r':  return '\r';
        case 't':  return '\t';
        case 'v':  return '\v';
        case '\\': return '\\';
        case '\"': return '\"';
        default:
            SL_ERR("The specified escape sequence (\\%c) is not currently "
                   "supported.",
                   escaped);
            return escaped;
    }
}

const char* byte2escaped(char byte) {
    switch (byte) {
        case '\a': return "\\a";
        case '\b': return "\\b";
        case '\e': return "\\e";
        case '\f': return "\\f";
        case '\n': return "\\n";
        case '\r': return "\\r";
        case '\t': return "\\t";
        case '\v': return "\\v";
        case '\\': return "\\\\";
        case '\"': return "\\\"";
        default:   return NULL;
    }
}
/* clang-format on */

void print_escaped_str(FILE* fp, const char* s) {
    SL_ASSERT(s != NULL);

    fputc('\"', fp);
    for (; *s != '\0'; s++) {
        const char* escape_sequence = byte2escaped(*s);
        if (escape_sequence != NULL)
            fprintf(fp, "%s", escape_sequence);
        else
            fprintf(fp, "%c", *s);
    }
    fputc('\"', fp);
}

/*----------------------------------------------------------------------------*/

bool sl_regex_match_groups(const char* pat, const char* str, bool ignore_case,
                           size_t* nmatch, regmatch_t** pmatch) {
    static regex_t r;

    int cflags = REG_EXTENDED;
    if (ignore_case)
        cflags |= REG_ICASE;

    if (regcomp(&r, pat, cflags) != REG_NOERROR) {
        SL_ERR("Failed to compile pattern \"%s\"", pat);
        return false;
    }

    /*
     * The size of the match array is the number of sub-expressions plus one
     * extra item for the entire match, which will be at index 0.
     */
    *nmatch = r.re_nsub + 1;
    *pmatch = mem_alloc(*nmatch * sizeof(regmatch_t*));

    const int code = regexec(&r, str, *nmatch, *pmatch, 0);
    regfree(&r);

    if (code != REG_NOERROR) {
        free(*pmatch);
        *nmatch = 0;
        *pmatch = NULL;
        return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

bool sl_concat_format(char** dst, size_t* dst_sz, size_t* dst_offset,
                      const char* fmt, ...) {
    va_list va;

    va_start(va, fmt);
    const int data_size = vsnprintf(NULL, 0, fmt, va);
    va_end(va);

    if (data_size < 0) {
        SL_ERR("Failed to get the target string length for C format: %s", fmt);
        return false;
    }

    if (*dst_offset + data_size + 1 >= *dst_sz) {
        *dst_sz = *dst_offset + data_size + 1;
        mem_realloc(*dst, *dst_sz);
    }

    char* real_dst = &(*dst)[*dst_offset];

    va_start(va, fmt);
    const int written = vsnprintf(real_dst, data_size + 1, fmt, va);
    va_end(va);

    *dst_offset += written;
    return true;
}

/*----------------------------------------------------------------------------*/

size_t int2str(long long x, char** dst) {
    /*
     * A call to `snprintf' with (NULL, 0, ...) as arguments can be used to get
     * the number of bytes to be written. We still have to add one for the
     * null-terminator.
     */
    const int size = snprintf(NULL, 0, "%lld", x);
    if (size < 0) {
        *dst = NULL;
        return 0;
    }

    *dst = mem_alloc(size + 1);
    snprintf(*dst, size + 1, "%lld", x);
    return size;
}

size_t flt2str(double x, char** dst) {
    const int size = snprintf(NULL, 0, "%f", x);
    if (size < 0) {
        *dst = NULL;
        return 0;
    }

    *dst = mem_alloc(size + 1);
    snprintf(*dst, size + 1, "%f", x);
    return size;
}
