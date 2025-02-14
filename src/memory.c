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

#define _XOPEN_SOURCE 500 /* strdup() */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "include/memory.h"
#include "include/error.h"

void* mem_alloc(size_t size) {
    void* result = malloc(size);
    if (result == NULL)
        SL_FATAL("Failed to allocate %zu bytes: %s (%d).",
                 size,
                 strerror(errno),
                 errno);
    return result;
}

void* mem_calloc(size_t nmemb, size_t size) {
    void* result = calloc(nmemb, size);
    if (result == NULL)
        SL_FATAL("Failed to allocate %zu elements of %zu bytes each: %s (%d).",
                 nmemb,
                 size,
                 strerror(errno),
                 errno);
    return result;
}

char* mem_strdup(const char* s) {
    char* result = strdup(s);
    if (result == NULL)
        SL_FATAL("Failed to copy string: %s (%d).", strerror(errno), errno);
    return result;
}
