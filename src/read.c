
#include <stdbool.h>
#include <stdio.h>

#include "include/util.h"
#include "include/read.h"
#include "include/lexer.h" /* is_token_separator() */

#define READ_BUFSZ 100

/*----------------------------------------------------------------------------*/

/*
 * Last character we read with `fgetc'. Useful to see if the last call to
 * `read' ended because of EOF.
 */
static int last_read = 0;

/*----------------------------------------------------------------------------*/

static bool is_comment_start(int c) {
    return c == ';';
}

static bool is_comment_end(int c) {
    return c == '\n';
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

char* read_expr(FILE* fp) {
    /*
     * If the last call to `read' was terminated by EOF, don't allocate anything
     * and return NULL to let the caller know.
     */
    if (last_read == EOF)
        return NULL;

    /* Will increase when encountering '(' and decrease with ')' */
    int nesting_level = 0;

    /* If true, we found a symbol/constant with nesting_level at 0 */
    bool isolated_symbol = false;

    size_t result_sz = READ_BUFSZ;
    char* result     = sl_safe_malloc(result_sz);
    size_t i         = 0;

    for (;;) {
        if (i >= result_sz - 1) {
            result_sz += READ_BUFSZ;
            sl_safe_realloc(result, result_sz);
        }

        const int c = get_next_non_comment(fp);
        last_read   = c;
        if (c == EOF)
            break;

        result[i++] = c;

        if (c == '(') {
            nesting_level++;
        } else if (c == ')') {
            /*
             * If we are still in level 0, we should have opened an expression.
             * TODO: If we add strings, we should check if ')' is inside one.
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
