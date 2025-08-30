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

#ifndef WARN_UNUSED_RESULT
#define WARN_UNUSED_RESULT __attribute__((malloc, warn_unused_result))
#endif /* WARN_UNUSED_RESULT */

/*
 * Allocate 'sz' bytes using stdlib's 'malloc' or 'calloc', ensuring a valid
 * pointer is returned. The caller is responsible for freeing the returned
 * pointer with 'mem_free'.
 */
void* mem_alloc(size_t sz) WARN_UNUSED_RESULT;
void* mem_calloc(size_t nmemb, size_t size) WARN_UNUSED_RESULT;

/*
 * Allocate a new string big enough to hold 's', and copy it. Ensures a valid
 * pointer is returned. The caller is responsible for freeing the returned
 * pointer with 'mem_free'.
 */
char* mem_strdup(const char* s) WARN_UNUSED_RESULT;

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

/*
 * Free the memory space pointed to by the specified pointer, which must have
 * been returned by a memory-allocation function. This function supports NULL as
 * its argument.
 */
void mem_free(void* ptr);

#endif /* MEMORY_H_ */
