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
 *
 * ---------------------------------------------------------------------------
 *
 * TODO: Rename these functions to "scanner", or something other than
 * reader? Since Lisp's `read' also parses the input into an `Expr'.
 */

#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

#include "include/util.h"
#include "include/read.h"
#include "include/lexer.h" /* is_token_separator() */

#define READ_BUFSZ 100

/*----------------------------------------------------------------------------*/

/*
 * Is the specified character a comment start/end delimiter?
 */
#define IS_COMMENT_START(C) ((c) == ';')
#define IS_COMMENT_END(C)   ((c) == '\n')

/*
 * Get the first non-comment character from `fp' using `fgetc'.  Doesn't
 * check for EOF, it returns it literally.
 */
static int get_next_non_comment(FILE* fp) {
    int c = fgetc(fp);

    while (IS_COMMENT_START(c)) {
        /* Skip comment */
        do {
            c = fgetc(fp);

            /* Make sure we never ignore EOF, even in comments */
            if (c == EOF)
                return EOF;
        } while (!IS_COMMENT_END(c));

        c = fgetc(fp);
    }

    return c;
}

/*----------------------------------------------------------------------------*/

/*
 * Read a double-quote-terminated string into the `dst' buffer, modifying its
 * size and position. Assumes the opening double-quote has just been written to
 * `dst'; reads up to the final non-escaped double-quote, included.
 *
 * The string will be reallocated if necessary, ensuring there is enough space
 * for the null terminator after the closing double-quote, but without actually
 * writing it.
 *
 * Returns true on success, or false if EOF was encountered (though note that
 * nothing is freed from here).
 */
static bool read_user_string(FILE* fp, char** dst, size_t* dst_sz,
                             size_t* dst_pos) {
    /*
     * Important notes:
     *   - There are no comments (starting with ';') in strings.
     *   - Null bytes are handled in the lexer, not here.
     *   - A backslash is used to escape. In this "read" stage, we don't have to
     *     check what is being escaped. Just store it literally.
     *   - A string ends as long as a non-escaped double-quote is found.
     *   - EOF might appear inside a string, in which case we should abort and
     *     return false.
     */
    int c = 0;
    while (c != '\"') {
        if (*dst_pos + 2 >= *dst_sz) {
            *dst_sz += READ_BUFSZ;
            sl_safe_realloc(*dst, *dst_sz);
        }

        c = fgetc(fp);
        if (c == EOF)
            return false;

        (*dst)[(*dst_pos)++] = c;

        if (c == '\\') {
            const int escaped = fgetc(fp);
            if (escaped == EOF)
                return false;

            (*dst)[(*dst_pos)++] = escaped;
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/

/*
 * Read a user list with the form "(...)". Assumes the caller just received an
 * opening parentheses, but didn't write it anywhere.
 */
static char* read_user_list(FILE* fp) {
    size_t result_pos = 0;
    size_t result_sz  = READ_BUFSZ;
    char* result      = sl_safe_malloc(result_sz);

    /* Will increase when encountering '(' and decrease with ')' */
    int nesting_level = 1;

    result[result_pos++] = '(';

    while (nesting_level > 0) {
        if (result_pos + 1 >= result_sz) {
            result_sz += READ_BUFSZ;
            sl_safe_realloc(result, result_sz);
        }

        const int c = get_next_non_comment(fp);
        if (c == EOF) {
            free(result);
            return NULL;
        }

        result[result_pos++] = c;

        /*
         * If we encounter an opening or closing parentheses, simply increase or
         * decrease the nesting level, respectively.
         *
         * If we encounter a double-quote, we should handle it similarly to
         * `read_isolated_user_string', since parentheses should be ignored
         * inside strings, escaped quotes should be handled, etc.
         */
        switch (c) {
            case '(': {
                nesting_level++;
            } break;

            case ')': {
                nesting_level--;
            } break;

            case '\"': {
                if (!read_user_string(fp, &result, &result_sz, &result_pos)) {
                    free(result);
                    return NULL;
                }
            } break;

            default:
                break;
        }
    }

    result[result_pos] = '\0';
    return result;
}

/*
 * Read an isolated user string.  Assumes the caller just received a
 * double-quote, but didn't write it anywhere. Writes the opening double-quote,
 * reads a string using the `read_user_string' function (used in other places),
 * checks if it contained EOF, and writes the final null terminator.
 */
static char* read_isolated_user_string(FILE* fp) {
    size_t result_pos = 0;
    size_t result_sz  = READ_BUFSZ;
    char* result      = sl_safe_malloc(result_sz);

    result[result_pos++] = '\"';

    if (!read_user_string(fp, &result, &result_sz, &result_pos)) {
        free(result);
        return NULL;
    }

    result[result_pos] = '\0';
    return result;
}

/*
 * Reads characters until a token separator is found.
 *
 * FIXME: On input "123(+ 1 2)", we should only read "123" on the first call,
 * and "(+ 1 2)" on the second. How do we store that token separator for next
 * calls. `g_incoming'?
 *
 * TODO: If using `g_incoming', remove `first_char' parameter.
 */
static char* read_isolated_atom(FILE* fp, char first_char) {
    size_t result_pos = 0;
    size_t result_sz  = READ_BUFSZ;
    char* result      = sl_safe_malloc(result_sz);

    result[result_pos++] = first_char;

    /*
     * Read until any token separator. This includes spaces, but also
     * parentheses, for example. The `is_token_separator' function is declared
     * in <lexer.h>.
     */
    for (;;) {
        if (result_pos + 1 >= result_sz) {
            result_sz += READ_BUFSZ;
            sl_safe_realloc(result, result_sz);
        }

        const int c = get_next_non_comment(fp);
        if (c == EOF) {
            free(result);
            return NULL;
        }

        if (is_token_separator(c))
            break;

        result[result_pos++] = c;
    }

    result[result_pos] = '\0';
    return result;
}

/*----------------------------------------------------------------------------*/

char* read_expr(FILE* fp) {
    /* Skip preceding spaces or comments, if any */
    int c;
    do {
        c = get_next_non_comment(fp);
    } while (isspace(c));

    if (c == EOF)
        return NULL;

    /*
     * The first character (which is guaranteed to not be EOF) will indicate
     * where we should stop parsing the current expression:
     *
     * - If we are opening a parentheses, we stop at the closing parentheses at
     *   that same level (that is not inside a string).
     * - If we are opening a double-quoted string, we stop at the first
     *   non-escaped double-quote.
     * - Otherwise, we are reading an isolated atom.
     *
     * We also check for some invalid characters:
     *
     * - If we encountered a closing parentheses in level 0, it is unmatched.
     */
    switch (c) {
        case '(':
            return read_user_list(fp);

        case ')':
            SL_ERR("Encountered unmatched ')'.");
            return sl_safe_strdup("");

        case '\"':
            return read_isolated_user_string(fp);

        default:
            return read_isolated_atom(fp, c);
    }
}
