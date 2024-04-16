
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/util.h"
#include "include/lexer.h"
#include "include/expr.h"
#include "include/parser.h"
#include "include/eval.h"
#include "include/env.h"

#define INPUT_BUFSZ 100

/* Allocate buffer and store a LISP expression in string format. Must be freed
 * by the caller. Returns true if it got EOF. */
static bool input_read(char** input) {
    *input          = malloc(INPUT_BUFSZ);
    size_t input_sz = INPUT_BUFSZ;

    /* Will increase when we encounter '(' and decrease with ')' */
    int nesting_level = 0;

    /* If true, we found a symbol/constant with nesting_level at 0 */
    bool isolated_symbol = false;

    char c;
    size_t i = 0;
    while ((c = getchar()) != EOF) {
        /* If we run out of space, allocate more */
        if (i >= input_sz) {
            input_sz += INPUT_BUFSZ;
            *input = realloc(*input, input_sz);
        }

        /* Store character in string */
        (*input)[i++] = c;

        if (c == '(') {
            nesting_level++;
        } else if (c == ')') {
            /* We are still in level 0, we should have opened an expression.
             * NOTE: This doesn't check if ')' is inside a string, comment,
             * etc. */
            if (nesting_level <= 0) {
                ERR("Encountered ')' before starting an expression.");
                break;
            }

            nesting_level--;

            /* We closed all the expressions we opened, we are done */
            if (nesting_level <= 0)
                break;
        } else if (nesting_level == 0) {
            /* We are reading outside of an expression */
            if (!isolated_symbol && !is_token_separator(c))
                /* We found a token outside of a list, it evaluates to itself */
                isolated_symbol = true;
            else if (isolated_symbol && is_token_separator(c))
                /* We just read an isolated symbol, and we found a separator */
                break;
        }
    }

    (*input)[i] = '\0';

    /* Return true if the last char was EOF, so the caller knows to not call us
     * again after it's done processing this expression. */
    return c == EOF;
}

int main(void) {
    /* Initialize global environment with symbols like "nil". Note that C
     * primitives are handled separately. */
    global_env = NULL;
    env_init(&global_env);

    bool got_eof = false;
    while (true) {
        if (got_eof)
            break;

        printf("sl> ");

        /* Allocate buffer and read an expression */
        char* input = NULL;
        got_eof     = input_read(&input);

        /* Get token array from input */
        Token* tokens = tokens_scan(input);

        /* We are done with the raw input, free it */
        free(input);

        /* Get root expression from token array */
        Expr* expr = parse(tokens);

        /* We are done with the token array, free it */
        tokens_free(tokens);

        if (expr == NULL)
            continue;

        /* Evaluate expression recursivelly */
        Expr* evaluated = eval(global_env, expr);

        /* We are done with the original expression */
        expr_free(expr);

        if (evaluated == NULL)
            continue;

        expr_print(evaluated);

        /* Free the evaluated expression */
        expr_free(evaluated);

        putchar('\n');
    }

    env_free(global_env);

    putchar('\n');
    return 0;
}
