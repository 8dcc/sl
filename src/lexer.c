
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h> /* pow() */

#include "include/lexer.h"
#include "include/util.h"

#define TOKEN_BUFSZ 100

/* Try to parse `str' as a decimal number, and store it in `out'. Returns true
 * if the conversion was successful. */
static bool parse_number(const char* str, double* out) {
    /* 0 means we are in the integer part of the number */
    int decimal_position = 0;

    /* First, clear the output */
    *out = 0.0;

    bool negate_result = false;
    if (str[0] == '-' && !is_token_separator(str[1])) {
        str++;
        negate_result = true;
    }

    while (!is_token_separator(*str)) {
        if (isdigit(*str)) {
            if (decimal_position == 0) {
                /* Shift 1 digit to the left and add new one */
                *out *= 10;
                *out += *str - '0';
            } else {
                /* '4' -> 0.004 */
                double tmp = (*str - '0') * pow(10.0, decimal_position);
                *out += tmp;

                /* If we added 0.01, add 0.001 next */
                decimal_position--;
            }
        } else if (*str == '.') {
            /* Found dot in the number, but we are already in the decimal part.
             * Error. */
            if (decimal_position != 0)
                return false;
            decimal_position--;
        } else {
            /* Unknown digit, not a number */
            return false;
        }

        str++;
    }

    if (negate_result)
        *out *= -1;

    return true;
}

/*
 * Try to scan a Token in the string pointed by `input_ptr', and increase the
 * pointer accordingly.
 *
 * If the end of the string is found, TOKEN_EOF is returned and `input_ptr' is
 * set to NULL.
 */
static Token get_token(char** input_ptr) {
    char* input = *input_ptr;

    /* Skip the spaces before the token, if any */
    while (isspace(*input))
        input++;

    Token result;

    /* Check for simple tokens */
    switch (*input) {
        case '(':
            result.type = TOKEN_LIST_OPEN;
            input++;
            goto done;

        case ')':
            result.type = TOKEN_LIST_CLOSE;
            input++;
            goto done;

        case '\'':
            result.type = TOKEN_QUOTE;
            input++;
            goto done;

        case '\0':
            result.type = TOKEN_EOF;
            input       = NULL;
            goto done;

        default:
            break;
    }

    /* Get the token length by looking for the next token separator */
    size_t token_sz;
    for (token_sz = 0; !is_token_separator(input[token_sz]); token_sz++)
        ;

    /* Temporarily terminate string at token separator, for parsing as number */
    char tmp        = input[token_sz];
    input[token_sz] = '\0';

    double parsed;
    if (parse_number(input, &parsed)) {
        /* Number (double) */
        result.type  = TOKEN_NUM;
        result.val.n = parsed;
    } else {
        /* Symbol (string) */
        result.type  = TOKEN_SYMBOL;
        result.val.s = sl_safe_malloc(token_sz + 1);
        memcpy(result.val.s, input, token_sz + 1);
    }

    /* Restore the token separator we had overwritten */
    input[token_sz] = tmp;

    /* Move the input pointer to the next token separator */
    input += token_sz;

done:
    /* Update the input pointer for the caller, and return the token */
    *input_ptr = input;
    return result;
}

/*----------------------------------------------------------------------------*/

Token* tokenize(char* input) {
    size_t tokens_num = TOKEN_BUFSZ;
    Token* tokens     = sl_safe_calloc(TOKEN_BUFSZ, sizeof(Token));

    for (size_t i = 0; input != NULL; i++) {
        if (i >= tokens_num) {
            tokens_num += TOKEN_BUFSZ;
            sl_safe_realloc(tokens, tokens_num * sizeof(Token));
        }

        /* Try to scan the token pointed to by `input', and increase the pointer
         * accordingly */
        tokens[i] = get_token(&input);
    }

    return tokens;
}

void tokens_free(Token* arr) {
    /* Free the strings we allocated in `token_store', if any */
    for (int i = 0; arr[i].type != TOKEN_EOF; i++)
        if (arr[i].type == TOKEN_SYMBOL)
            free(arr[i].val.s);

    /* Once we are done freeing all the allocations we made inside the array,
     * free the array itself */
    free(arr);
}

bool is_token_separator(char c) {
    return isspace(c) || c == '\0' || c == '(' || c == ')';
}

void tokens_print(Token* arr) {
    printf("[ ");

    while (arr->type != TOKEN_EOF) {
        switch (arr->type) {
            case TOKEN_NUM:
                printf("%f, ", arr->val.n);
                break;

            case TOKEN_SYMBOL:
                printf("\"%s\", ", arr->val.s);
                break;

            case TOKEN_LIST_OPEN:
                printf("LIST_OPEN, ");
                break;

            case TOKEN_LIST_CLOSE:
                printf("LIST_CLOSE, ");
                break;

            case TOKEN_QUOTE:
                printf("QUOTE, ");
                break;

            case TOKEN_EOF:
                printf("???");
                break;
        }

        arr++;
    }

    printf("EOF ]\n");
}
