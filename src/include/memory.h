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

/*
 * Allocate 'sz' bytes using stdlib's 'malloc' or 'calloc', ensuring a valid
 * pointer is returned.
 */
void* mem_alloc(size_t sz) __attribute__((malloc, warn_unused_result));
void* mem_calloc(size_t nmemb, size_t size)
  __attribute__((malloc, warn_unused_result));

/*
 * Allocate a new string big enough to hold 's', and copy it. Ensures a valid
 * pointer is returned.
 */
char* mem_strdup(const char* s) __attribute__((malloc, warn_unused_result));

/*
 * Change the size of the memory block pointed to by the pointer that
 * 'double_pointer' points to. Uses stdlib's 'realloc', ensuring a valid
 * pointer is returned.
 *
 * Please note that, although the type of the 'double_ptr' parameter is
 * 'void*', it's actually a double pointer, so the call to this function should
 * look something like:
 *
 *    int* ptr = calloc(5, sizeof(int));
 *    ptr[4] = 123;
 *    mem_realloc(&ptr, 10);
 *    ptr[9] = 456;
 */
void mem_realloc(void* double_ptr, size_t new_size);

#endif /* MEMORY_H_ */
