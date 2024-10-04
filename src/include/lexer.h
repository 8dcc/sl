#ifndef LEXER_H_
#define LEXER_H_ 1

#include <stdio.h> /* FILE */

enum ETokenTypes {
    /* Used to indicate the end of a Token array */
    TOKEN_EOF,

    /* The following make use of Token.val */
    TOKEN_NUM_INT, /* Number (long long) */
    TOKEN_NUM_FLT, /* Number (double) */
    TOKEN_SYMBOL,  /* Symbol (string) */
    TOKEN_STRING,  /* String (string) */

    /* The rest don't make use of Token.val */
    TOKEN_LIST_OPEN,
    TOKEN_LIST_CLOSE,

    /* Indicates that the next expression should be wrapped in (quote ...) */
    TOKEN_QUOTE,
};

typedef struct Token {
    enum ETokenTypes type;
    union {
        long long n;
        double f;
        char* s;
    } val;
} Token;

/*----------------------------------------------------------------------------*/

/* Allocate and fill an array of Tokens from the input. Must be freed by the
 * caller. */
Token* tokenize(char* input);

/* Free an array of tokens, along with all the memory used by each element */
void tokens_free(Token* arr);

/* Is `c' a token separator? Used by `get_token' and `read_expr'. */
bool is_token_separator(char c);

/* Print all tokens in a Token array terminated by TOKEN_EOF */
void tokens_print(FILE* fp, Token* arr);

#endif /* LEXER_H_ */
