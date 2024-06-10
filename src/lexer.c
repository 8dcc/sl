
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h> /* pow */
#include "include/lexer.h"
#include "include/util.h"

#define TOKEN_BUFSZ 50

static bool parse_number(const char* str, double* out) {
    /* 0 means we are in the integer part of the number */
    int decimal_position = 0;

    /* First, clear the output */
    *out = 0.0;

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

    return true;
}

/* Scan and store the next token from `in' to `out'. Returns the next character
 * after the token, or NULL if the next char is the end of the string. */
static char* token_store(Token* out, char* in) {
    /* Skip spaces, if any */
    while (isspace(*in))
        in++;

    /* Check for simple tokens */
    switch (*in) {
        case '(':
            out->type = TOKEN_LIST_OPEN;
            return in + 1;
        case ')':
            out->type = TOKEN_LIST_CLOSE;
            return in + 1;
        case '\'':
            out->type = TOKEN_QUOTE;
            return in + 1;
        case '\0':
            out->type = TOKEN_EOF;
            return NULL;
        default:
            break;
    }

    /* Go until next token separator */
    size_t i;
    for (i = 0; !is_token_separator(in[i]); i++)
        ;

    /* Temporarily terminate string at token separator, for parsing as number */
    char tmp = in[i];
    in[i]    = '\0';

    double parsed;
    if (parse_number(in, &parsed)) {
        /* Number (double) */
        out->type  = TOKEN_NUM;
        out->val.n = parsed;
    } else {
        /* Symbol (string) */
        out->type = TOKEN_SYMBOL;

        out->val.s = malloc(i + 1);
        SL_ASSERT_ALLOC(out->val.s);

        strcpy(out->val.s, in);
    }

    /* Restore the token separator we had overwritten */
    in[i] = tmp;

    /* Return next char */
    return &in[i];
}

bool is_token_separator(char c) {
    return isspace(c) || c == '\0' || c == '(' || c == ')';
}

Token* tokens_scan(char* input) {
    size_t tokens_num = TOKEN_BUFSZ;
    Token* tokens     = calloc(TOKEN_BUFSZ, sizeof(Token));

    for (size_t i = 0; input != NULL; i++) {
        if (i >= tokens_num) {
            tokens_num += TOKEN_BUFSZ;
            tokens = realloc(tokens, tokens_num * sizeof(Token));
        }

        input = token_store(&tokens[i], input);
    }

    return tokens;
}

void tokens_free(Token* arr) {
    for (int i = 0; arr[i].type != TOKEN_EOF; i++) {
        if (arr[i].type == TOKEN_SYMBOL) {
            /* Free the string we allocated in token_store() */
            free(arr[i].val.s);
        }
    }

    /* Once we are done freeing all the allocations we made inside the array,
     * free the array itself */
    free(arr);
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
            default:
                fprintf(stderr, "UNKNOWN, ");
                break;
        }

        arr++;
    }

    printf("EOF ]\n");
}
