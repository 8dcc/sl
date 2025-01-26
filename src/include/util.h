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

#ifndef UTIL_H_
#define UTIL_H_ 1

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h> /* FILE */
#include <regex.h> /* regmatch_t */

#include "lisp_types.h" /* LispInt, LispFlt */

/*----------------------------------------------------------------------------*/

/* See strtoll(3) */
#define STRTOLL_ANY_BASE 0

#define LENGTH(ARR) ((int)(sizeof(ARR) / sizeof((ARR)[0])))

#define MIN(A, B)        ((A) < (B) ? (A) : (B))
#define MAX(A, B)        ((A) > (B) ? (A) : (B))
#define CLAMP(N, LO, HI) (MIN(MAX((LO), (N)), (HI)))

/*
 * Avoid -Wunused-parameter
 */
#define SL_UNUSED(VAR) ((void)VAR)

/*----------------------------------------------------------------------------*/

/*
 * Return the actual value of an escape sequence character. If the character is
 * not part of a supported escape sequence, an error message is printed to
 * 'stderr' and the character is returned unchanged.
 *
 * For example, 'n' -> 0xA (\n).
 */
char escaped2byte(char escaped);

/*
 * Return a two-character escape sequence that represents the specified byte. If
 * the input is not the value of a supported escape sequence, NULL is returned.
 *
 * For example, 0xA -> "\n" (that is, '\\' and 'n').
 */
const char* byte2escaped(char byte);

/*
 * Print a string with values corresponding to escape sequences. The printed
 * string should evaluate to the input.
 */
void print_escaped_str(FILE* fp, const char* s);

/*----------------------------------------------------------------------------*/

/*
 * Find all matches of 'pat' in 'str', writing the number of sub-expression
 * matches in 'nmatch' and writing an array of 'nmatch + 1' elements in
 * 'pmatch'. See regexec(3) for more information.
 *
 * The function returns true if the pattern compilation succedeed and there was
 * a match. If (and only if) true is returned, the caller is responsible for
 * freeing 'pmatch'.
 */
bool sl_regex_match_groups(const char* pat, const char* str, bool ignore_case,
                           size_t* nmatch, regmatch_t** pmatch);

/*----------------------------------------------------------------------------*/

/*
 * Concatenate formatted data into an existing string, at an specific offset.
 *
 * The 'dst' argument should point to a reallocable string of size
 * '*dst_sz'. The value at 'dst_offset' is the location in the string where the
 * data will be written.
 *
 * If the new formatted data would write past '*dst_sz', 'dst' is reallocated
 * and '*dst_sz' is updated accordingly.
 *
 * The function will update '*dst_offset' so it marks the position in '*dst'
 * where we finished writing. In other words, the position of the null
 * terminator.
 *
 * The function returns 'true' on success, or 'false' if an error was printed.
 */
bool sl_concat_format(char** dst, size_t* dst_sz, size_t* dst_offet,
                      const char* fmt, ...)
  __attribute__((format(printf, 4, 5)));

/*----------------------------------------------------------------------------*/

/*
 * Allocate a string in '*dst' big enough to store the representation of the
 * integer 'x', and convert it. The allocated string must be freed by the
 * caller.
 *
 * Returns the size of the allocated string. On failure, '*dst' is set to NULL
 * and zero is returned.
 */
size_t int2str(LispInt x, char** dst);

/*
 * Allocate a string in '*dst' big enough to store the representation of the
 * float 'x', and convert it. The allocated string must be freed by the caller.
 *
 * Returns the size of the allocated string. On failure, '*dst' is set to NULL
 * and zero is returned.
 */
size_t flt2str(LispFlt x, char** dst);

#endif /* UTIL_H_ */
