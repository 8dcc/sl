
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "include/util.h"
#include "include/lexer.h"

#define TOKEN_BUFSZ  100
#define STRING_BUFSZ 100

/*
 * Read the user input and store it in an allocated string, parsing the
 * supported escape sequences. Returns the number of parsed characters from the
 * input, including the final double quote.
 */
static size_t parse_user_string(const char* input, char** dst) {
    SL_ASSERT(input[0] == '\"');

    size_t result_pos = 0;
    size_t result_sz  = STRING_BUFSZ;
    char* result      = sl_safe_malloc(result_sz);

    size_t input_pos;
    for (input_pos = 1; input[input_pos] != '\"'; input_pos++, result_pos++) {
        if (input[input_pos] == '\0') {
            SL_ERR("Null bytes are not currently supported in strings. "
                   "Stopping early.");
            break;
        }

        if (result_pos >= result_sz - 1) {
            result_sz += TOKEN_BUFSZ;
            sl_safe_realloc(result, result_sz);
        }

        /* Parse escape sequences */
        if (input[input_pos] == '\\') {
            input_pos++;
            result[result_pos] = escaped2byte(input[input_pos]);
            continue;
        }

        result[result_pos] = input[input_pos];
    }

    result[result_pos] = '\0';
    *dst               = result;
    return input_pos + 1;
}

/*
 * Set the value and type of the token based on the input. Only checks for
 * integers, floats or symbols.
 */
static void set_value_from_str(Token* dst, char* str) {
    char* endptr;

    /* Try to fully convert the string into a `long long' using `strtoll' */
    long long int_num = strtoll(str, &endptr, STRTOLL_ANY_BASE);
    if (endptr != NULL && str != endptr && *endptr == '\0') {
        dst->type  = TOKEN_NUM_INT;
        dst->val.n = int_num;
        return;
    }

    /* Try to fully convert the string into a `double' using `strtod' */
    double flt_num = strtod(str, &endptr);
    if (endptr != NULL && str != endptr && *endptr == '\0') {
        dst->type  = TOKEN_NUM_FLT;
        dst->val.f = flt_num;
        return;
    }

    /* If we couldn't convert it to a `double' or a `long long', assume it's a
     * symbol. */
    dst->type  = TOKEN_SYMBOL;
    dst->val.s = sl_safe_strdup(str);
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

    /* Check for simple tokens and string openings */
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

        case '`':
            result.type = TOKEN_BACKQUOTE;
            input++;
            goto done;

        case ',':
            result.type = TOKEN_COMMA;
            input++;
            goto done;

        case '\0':
            result.type = TOKEN_EOF;
            input       = NULL;
            goto done;

        case '\"':
            result.type = TOKEN_STRING;
            input += parse_user_string(input, &result.val.s);
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

    /* Set the value and type of the token based on the input. */
    set_value_from_str(&result, input);

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
         * accordingly. */
        tokens[i] = get_token(&input);
    }

    return tokens;
}

void tokens_free(Token* arr) {
    for (int i = 0; arr[i].type != TOKEN_EOF; i++)
        if (arr[i].type == TOKEN_SYMBOL || arr[i].type == TOKEN_STRING)
            free(arr[i].val.s);

    free(arr);
}

bool is_token_separator(char c) {
    return isspace(c) || c == '\0' || c == '(' || c == ')';
}

void tokens_print(FILE* fp, Token* arr) {
    fprintf(fp, "[ ");

    while (arr->type != TOKEN_EOF) {
        switch (arr->type) {
            case TOKEN_NUM_INT:
                fprintf(fp, "%lld, ", arr->val.n);
                break;

            case TOKEN_NUM_FLT:
                fprintf(fp, "%f, ", arr->val.f);
                break;

            case TOKEN_SYMBOL:
                fprintf(fp, "\"%s\", ", arr->val.s);
                break;

            case TOKEN_STRING:
                print_escaped_str(fp, arr->val.s);
                fprintf(fp, ", ");
                break;

            case TOKEN_LIST_OPEN:
                fprintf(fp, "LIST_OPEN, ");
                break;

            case TOKEN_LIST_CLOSE:
                fprintf(fp, "LIST_CLOSE, ");
                break;

            case TOKEN_QUOTE:
                fprintf(fp, "QUOTE, ");
                break;

            case TOKEN_BACKQUOTE:
                fprintf(fp, "BACKQUOTE, ");
                break;

            case TOKEN_COMMA:
                fprintf(fp, "TOKEN_COMMA, ");
                break;

            case TOKEN_EOF:
                fprintf(fp, "???");
                break;
        }

        arr++;
    }

    fprintf(fp, "EOF ]\n");
}
