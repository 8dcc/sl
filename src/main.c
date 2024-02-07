
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/util.h"
#include "include/lexer.h"
#include "include/parser.h"
#include "include/eval.h"

#define INPUT_BUFSZ 100

/* Allocate buffer and store a LISP expression. Must be freed by the caller */
static char* input_read(void) {
    /* Used to indicate the caller that we got EOF, but after sending the
    expression that contained it. */
    static bool last_call_was_eof = false;
    if (last_call_was_eof)
        return NULL;

    size_t input_sz = INPUT_BUFSZ;
    char* input     = malloc(INPUT_BUFSZ);

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
            input = realloc(input, input_sz);
        }

        /* Store character in string */
        input[i++] = c;

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

    /* Store in static var so we can return NULL on next call */
    if (c == EOF)
        last_call_was_eof = true;

    input[i] = '\0';
    return input;
}

int main(void) {
    while (true) {
        printf("sl> ");

        char* input = input_read();
        if (input == NULL)
            break;

        /* Get token array from input */
        Token* tokens = tokens_scan(input);

        /* We are done with the raw input, free it */
        free(input);

        /* Get root expression from token array */
        Expr* expr = parse(tokens);

        /* We are done with the token array, free it */
        free(tokens);

        if (expr == NULL)
            continue;

        /* Evaluate expression recursivelly */
        Expr* evaluated = eval(expr);

        /* We are done with the original expression */
        expr_free(expr);

        if (evaluated == NULL)
            continue;

        expr_print(evaluated);

        /* Free the evaluated expression */
        expr_free(evaluated);

        putchar('\n');
    }

    putchar('\n');
    return 0;
}
