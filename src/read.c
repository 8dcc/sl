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
#define IS_COMMENT_START(C) ((C) == ';')
#define IS_COMMENT_END(C)   ((C) == '\n')

/*
 * Last character read by `store_incoming', which will be returned on the next
 * call to `get_user_char'.
 */
static int g_incoming = ' ';

/*
 * Get a user character from `fp', and store it in `g_incoming' return the
 * previous value from `g_incoming'. Does not handle EOF in any way.
 *
 * We use this "delayed" approach to allow the caller to check the next incoming
 * character.
 */
static int get_user_char(FILE* fp) {
    int c      = g_incoming;
    g_incoming = fgetc(fp);
    return c;
}

/*
 * Read characters from `fp' until a non-comment is found in `g_incoming',
 * without actually reading it. Uses `get_user_char'.
 *
 * Returns false if EOF was encountered before the non-comment, or true
 * otherwise.
 */
static bool read_until_incoming_non_comment(FILE* fp) {
    while (IS_COMMENT_START(g_incoming)) {
        /* Skip comment contents */
        do {
            get_user_char(fp);
            if (g_incoming == EOF)
                return false;
        } while (!IS_COMMENT_END(g_incoming));

        /* Skip incoming comment end */
        get_user_char(fp);
    }

    return true;
}

/*
 * Get the first non-comment character from `fp' using `get_user_char'. If EOF
 * is encountered inside a comment, it is returned literally.
 */
static int get_next_non_comment(FILE* fp) {
    if (!read_until_incoming_non_comment(fp))
        return EOF;

    return get_user_char(fp);
}

/*----------------------------------------------------------------------------*/

/*
 * Read a double-quote-terminated string into the `dst' buffer, modifying its
 * size and position. Assumes the opening double-quote has just been written to
 * `dst'; and reads up to the final non-escaped double-quote, included.
 *
 * The string will be reallocated if necessary, ensuring there is enough space
 * for the null terminator after the closing double-quote, but without actually
 * writing it.
 */
static void read_user_string(FILE* fp, char** dst, size_t* dst_sz,
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
    while (c != '\"' && g_incoming != EOF) {
        if (*dst_pos + 2 >= *dst_sz) {
            *dst_sz += READ_BUFSZ;
            sl_safe_realloc(*dst, *dst_sz);
        }

        (*dst)[(*dst_pos)++] = c = get_user_char(fp);

        if (c == '\\' && g_incoming != EOF)
            (*dst)[(*dst_pos)++] = get_user_char(fp);
    }
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

    SL_ASSERT(g_incoming == '(');
    result[result_pos++] = get_next_non_comment(fp);

    while (nesting_level > 0 && g_incoming != EOF) {
        if (result_pos + 1 >= result_sz) {
            result_sz += READ_BUFSZ;
            sl_safe_realloc(result, result_sz);
        }

        const int c          = get_next_non_comment(fp);
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
                read_user_string(fp, &result, &result_sz, &result_pos);
            } break;

            default:
                break;
        }
    }

    result[result_pos] = '\0';
    return result;
}

/*
 * Read an isolated user string. Assumes the caller just received a
 * double-quote, but didn't write it anywhere. Writes the opening double-quote,
 * reads a string using the `read_user_string' function (used in other places),
 * and writes the final null terminator.
 */
static char* read_isolated_user_string(FILE* fp) {
    size_t result_pos = 0;
    size_t result_sz  = READ_BUFSZ;
    char* result      = sl_safe_malloc(result_sz);

    SL_ASSERT(g_incoming == '\"');
    result[result_pos++] = get_next_non_comment(fp);

    read_user_string(fp, &result, &result_sz, &result_pos);
    result[result_pos] = '\0';
    return result;
}

/*
 * Reads characters until a token separator is found.
 */
static char* read_isolated_atom(FILE* fp) {
    size_t result_pos = 0;
    size_t result_sz  = READ_BUFSZ;
    char* result      = sl_safe_malloc(result_sz);

    /*
     * Read until the incoming character is a token separator. This includes
     * spaces, but also parentheses, for example. The `is_token_separator'
     * function is declared in <lexer.h>.
     */
    while (!is_token_separator(g_incoming) && g_incoming != EOF) {
        if (result_pos + 1 >= result_sz) {
            result_sz += READ_BUFSZ;
            sl_safe_realloc(result, result_sz);
        }

        result[result_pos++] = get_next_non_comment(fp);
    }

    result[result_pos] = '\0';
    return result;
}

/*----------------------------------------------------------------------------*/

char* read_expr(FILE* fp) {
    /* Skip preceding spaces or comments, if any */
    while (isspace(g_incoming) || IS_COMMENT_START(g_incoming)) {
        get_user_char(fp);
        read_until_incoming_non_comment(fp);
    }

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
     * - If the next character is EOF, inform the caller that there is no more
     *   user input. We also clear the `g_incoming' variable for subsequent
     *   calls.
     */
    switch (g_incoming) {
        case '(':
            return read_user_list(fp);

        case '\"':
            return read_isolated_user_string(fp);

        default:
            return read_isolated_atom(fp);

        case ')':
            SL_ERR("Encountered unmatched ')'.");
            get_next_non_comment(fp);
            return read_expr(fp);

        case EOF:
            g_incoming = ' ';
            return NULL;
    }
}
