
#include <stdbool.h>
#include <stdio.h>

#include "include/util.h"
#include "include/read.h"
#include "include/lexer.h" /* is_token_separator() */

#define READ_BUFSZ 100

/*----------------------------------------------------------------------------*/

static bool is_comment_start(int c) {
    return c == ';';
}

static bool is_comment_end(int c) {
    return c == '\n';
}

/*
 * Did the character at `str[pos]' just open/close a string with a double-quote?
 * Checks if it was escaped:
 *
 *   (...")     -> true
 *   (...\\\")  -> false (odd '\')
 *   (...\\\\") -> true (even '\')
 */
static bool just_toggled_string_state(const char* str, int pos) {
    if (str[pos] != '\"')
        return false;
    pos--;

    /* Every consecutive backslash from the end, toggle the variable to store if
     * the number is odd or even. */
    bool odd_backslashes = true;
    for (; pos >= 0 && str[pos] == '\\'; pos--)
        odd_backslashes = !odd_backslashes;

    return odd_backslashes;
}

/* Return the first non-comment character from `fp' */
static int get_next_non_comment(FILE* fp) {
    int c = fgetc(fp);

    while (is_comment_start(c)) {
        /* Skip comment */
        do {
            c = fgetc(fp);

            /* Make sure we never ignore EOF, even in comments */
            if (c == EOF)
                return EOF;
        } while (!is_comment_end(c));

        c = fgetc(fp);
    }

    return c;
}

/*----------------------------------------------------------------------------*/

char* read_expr(FILE* fp) {
    /* Will increase when encountering '(' and decrease with ')' */
    int nesting_level = 0;

    /* If true, we found a symbol/constant with nesting_level at 0 */
    bool isolated_symbol = false;

    /* If true, we are inside a user string in the form "..." */
    bool inside_string = false;

    size_t result_sz = READ_BUFSZ;
    char* result     = sl_safe_malloc(result_sz);
    size_t i         = 0;

    for (;;) {
        if (i >= result_sz - 1) {
            result_sz += READ_BUFSZ;
            sl_safe_realloc(result, result_sz);
        }

        const int c = get_next_non_comment(fp);
        if (c == EOF) {
            free(result);
            return NULL;
        }

        result[i++] = c;

        /*
         * First, check if we are opening/closing a string. That static function
         * will check for escaped double-quotes.
         */
        if (just_toggled_string_state(result, i - 1))
            inside_string = !inside_string;

        /*
         * If we are inside a string, we don't want to check anything else until
         * we close it.
         */
        if (inside_string)
            continue;

        if (c == '(') {
            nesting_level++;
        } else if (c == ')') {
            /*
             * If we are still in level 0, we should have opened an expression.
             *
             * Otherwise, if we closed all the expressions that we opened, we
             * are done.
             */
            if (nesting_level <= 0) {
                ERR("Encountered unmatched ')'.");
                break;
            }

            nesting_level--;
            if (nesting_level <= 0)
                break;
        } else if (nesting_level == 0) {
            /*
             * We are reading outside of an expression. If we weren't reading an
             * isolated atom and this isn't a token separator, start reading the
             * atom. Otherwise, if we were reading an isolated atom and we
             * reached a token separator, we are done.
             */
            if (!isolated_symbol && !is_token_separator(c))
                isolated_symbol = true;
            else if (isolated_symbol && is_token_separator(c))
                break;
        }
    }

    result[i] = '\0';
    return result;
}
