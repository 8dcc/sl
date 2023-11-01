
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "include/util.h"

#define INPUT_BUFSZ 100

/* Allocate buffer and store a LISP expression. Must be freed by the caller */
static char* input_read(void) {
    static bool last_call_was_eof = false;
    if (last_call_was_eof)
        return NULL;

    size_t input_sz = INPUT_BUFSZ;
    char* input     = malloc(INPUT_BUFSZ);

    /* Will increase when we encounter '(' and decrease with ')' */
    int nesting_level = 0;

    char c;
    size_t i;
    for (i = 0; (c = getchar()) != EOF; i++) {
        /* If we run out of space, allocate more */
        if (i >= input_sz) {
            input_sz += INPUT_BUFSZ;
            input = realloc(input, input_sz);
        }

        /* Store character in string */
        input[i] = c;

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
        printf("(sl) ");

        char* input = input_read();
        if (input == NULL)
            break;

        /* TODO */
        printf("%s\n", input);
        printf("--------------------------------------");

        free(input);
    }

    return 0;
}
