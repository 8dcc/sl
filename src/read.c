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
 * reader? Since Lisp's `read' also parses the input into an 'Expr'.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "include/util.h"
#include "include/memory.h"
#include "include/error.h"
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
 * Get the incoming character from 'fp'. Calls 'fgetc' and, if it's not EOF,
 * calls 'ungetc'.
 */
static int get_incoming(FILE* fp) {
    int incoming = fgetc(fp);
    if (incoming != EOF)
        ungetc(incoming, fp);
    return incoming;
}

/*
 * Read characters from 'fp' until a non-comment is found using 'get_incoming',
 * without actually reading it.
 *
 * Returns false if EOF was encountered before the non-comment, or true
 * otherwise.
 */
static bool read_until_incoming_non_comment(FILE* fp) {
    int c;

    while (IS_COMMENT_START(get_incoming(fp))) {
        /* Skip comment contents, along with comment end */
        do {
            c = fgetc(fp);
            if (c == EOF)
                return false;
        } while (!IS_COMMENT_END(c));
    }

    return true;
}

/*
 * Get the first non-comment character from 'fp' using 'fgetc'. If EOF is
 * encountered inside a comment, it is returned literally.
 */
static int get_next_non_comment(FILE* fp) {
    if (!read_until_incoming_non_comment(fp))
        return EOF;

    return fgetc(fp);
}

/*----------------------------------------------------------------------------*/

/*
 * Read a double-quote-terminated string into the 'dst' buffer, modifying its
 * size and position. Assumes the opening double-quote has just been written to
 * 'dst'; and reads up to the final non-escaped double-quote, included.
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
    while (c != '\"') {
        if (*dst_pos + 1 >= *dst_sz) {
            *dst_sz += READ_BUFSZ;
            mem_realloc(dst, *dst_sz);
        }

        c = fgetc(fp);
        if (c == EOF)
            break;
        (*dst)[(*dst_pos)++] = c;

        if (c == '\\') {
            const int escaped = fgetc(fp);
            if (c == EOF)
                break;
            (*dst)[(*dst_pos)++] = escaped;
        }
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
    char* result      = mem_alloc(result_sz);

    /* Will increase when encountering '(' and decrease with ')' */
    int nesting_level = 1;

    SL_ASSERT(get_incoming(fp) == '(');
    result[result_pos++] = get_next_non_comment(fp);

    while (nesting_level > 0) {
        if (result_pos + 1 >= result_sz) {
            result_sz += READ_BUFSZ;
            mem_realloc(&result, result_sz);
        }

        const int c = get_next_non_comment(fp);
        if (c == EOF)
            break;
        result[result_pos++] = c;

        /*
         * If we encounter an opening or closing parentheses, simply increase or
         * decrease the nesting level, respectively.
         *
         * If we encounter a double-quote, we should handle it similarly to
         * 'read_isolated_user_string', since parentheses should be ignored
         * inside strings, escaped quotes should be handled, etc.
         */
        switch (c) {
            case '(':
                nesting_level++;
                break;

            case ')':
                nesting_level--;
                break;

            case '\"':
                read_user_string(fp, &result, &result_sz, &result_pos);
                break;

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
 * reads a string using the 'read_user_string' function (used in other places),
 * and writes the final null terminator.
 */
static char* read_isolated_user_string(FILE* fp) {
    size_t result_pos = 0;
    size_t result_sz  = READ_BUFSZ;
    char* result      = mem_alloc(result_sz);

    SL_ASSERT(get_incoming(fp) == '\"');
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
    char* result      = mem_alloc(result_sz);

    /*
     * Read until the incoming character is a token separator. This includes
     * spaces, but also parentheses, for example. The 'is_token_separator'
     * function is declared in <lexer.h>.
     */
    for (;;) {
        const int incoming = get_incoming(fp);
        if (is_token_separator(incoming) || incoming == EOF)
            break;

        if (result_pos + 1 >= result_sz) {
            result_sz += READ_BUFSZ;
            mem_realloc(&result, result_sz);
        }

        result[result_pos++] = get_next_non_comment(fp);
    }

    result[result_pos] = '\0';
    return result;
}

/*
 * Read an expression, starting with a quote-like character. Assumes the caller
 * just received a quote, but did not write it anywhere.
 *
 * First, we store the quote character, then we read an expression
 * (independently of the type) and we prepend the character we stored to the
 * string. Technically, this uses more allocations than necessary, but it keeps
 * the code clean and modular.
 */
static char* read_quoted_expr(FILE* fp) {
    const char quote_char = get_next_non_comment(fp);

    char* expr_str        = read_expr(fp);
    const size_t expr_len = strlen(expr_str);

    char* result = mem_alloc(1 + expr_len + 1);
    result[0]    = quote_char;
    memcpy(&result[1], expr_str, expr_len + 1);

    free(expr_str);
    return result;
}

/*----------------------------------------------------------------------------*/

char* read_expr(FILE* fp) {
    int incoming = get_incoming(fp);

    /* Skip leading spaces or comments, if any */
    while (isspace(incoming) || IS_COMMENT_START(incoming)) {
        fgetc(fp);
        read_until_incoming_non_comment(fp);
        incoming = get_incoming(fp);
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
     *   user input.
     */
    switch (incoming) {
        case '(':
            return read_user_list(fp);

        case '\"':
            return read_isolated_user_string(fp);

        default:
            return read_isolated_atom(fp);

        case '\'':
        case '`':
        case ',':
            return read_quoted_expr(fp);

        case ')':
            SL_ERR("Encountered unmatched ')'.");
            get_next_non_comment(fp);
            return read_expr(fp);

        case EOF:
            return NULL;
    }
}
