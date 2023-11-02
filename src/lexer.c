
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "include/lexer.h"
#include "include/util.h"

#define TOKEN_BUFSZ 50

/* Used by token_store() */
static inline bool is_token_separator(char c) {
    return isspace(c) || c == '\0' || c == '(' || c == ')';
}

/* Scan and store the next token from `in` to `out`. Returns the netx character
 * after the token, or NULL if the next char is the end of the string. */
static char* token_store(Token* out, char* in) {
    size_t i = 0;

    /* Skip spaces, if any */
    while (isspace(in[i]))
        i++;

    /* TODO: Might be a good idea to iterate until separator, temporarily end
     * the string there, and use regex to check what type of token it is. */

    /* TODO: Check for TOKEN_EOF ('\0'), TOKEN_LIST_OPEN, TOKEN_LIST_CLOSE */

    /* Iterate until next token separator, and check if the token was a digit */
    bool only_digits = true;
    while (!is_token_separator(in[i])) {
        if (only_digits && !isdigit(in[i]))
            only_digits = false;

        i++;
    }

    /* TODO: Floats */
    if (only_digits) {
        out->type = TOKEN_NUM;

        /* We need to temporarily terminate string on the separator for atoi */
        char tmp   = in[i];
        in[i]      = '\0';
        out->val.i = atoi(in);
        in[i]      = tmp;
    } else {
        out->type  = TOKEN_SYM;
        out->val.s = malloc(i + 1);

        if (out->val.s == NULL) {
            ERR("Error allocating token string. Aborting...");
            exit(1);
        }

        strncpy(out->val.s, in, i);
        out->val.s[i] = '\0';
    }

    /* Return NULL if we are done with the input, or next char otherwise */
    return (in[i] == '\0') ? NULL : &in[i];
}

Token* scan_tokens(char* input) {
    size_t tokens_num = TOKEN_BUFSZ;
    Token* tokens     = calloc(TOKEN_BUFSZ, sizeof(Token));

    /* TODO */

    return NULL;
}
