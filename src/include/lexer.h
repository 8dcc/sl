#ifndef LEXER_H_
#define LEXER_H_ 1

enum ETokenTypes {
    /* Used to indicate the end of a Token array */
    TOKEN_EOF,

    /* The following make use of Token.val */
    TOKEN_NUM,    /* Number (double) */
    TOKEN_SYMBOL, /* Symbol (string) */

    /* The rest don't make use of Token.val */
    TOKEN_LIST_OPEN,
    TOKEN_LIST_CLOSE,

    /* Indicates that the next expression should be wrapped in (quote ...) */
    TOKEN_QUOTE,
};

typedef struct Token {
    enum ETokenTypes type;
    union {
        double n;
        char* s;
    } val;
} Token;

/*----------------------------------------------------------------------------*/

/* Allocate and fill an array of Tokens from the input. Must be freed by the
 * caller. */
Token* tokens_scan(char* input);

/* Free an array of tokens, along with all the memory used by each element */
void tokens_free(Token* arr);

/* Is `c' a token separator? Used by `get_token' and `input_read'. */
bool is_token_separator(char c);

/* Print all tokens in a Token array terminated by TOKEN_EOF */
void tokens_print(Token* arr);

#endif /* LEXER_H_ */
