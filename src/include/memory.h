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

#ifndef MEMORY_H_
#define MEMORY_H_ 1

#include <stddef.h>
#include <stdlib.h> /* realloc() */

#include "error.h" /* SL_FATAL() */

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

#endif /* MEMORY_H_ */
