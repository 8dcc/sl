#ifndef LEXER_H_
#define LEXER_H_ 1

enum ETokenTypes {
    TOKEN_EOF,

    /* The following make use of Token.val */
    TOKEN_NUM,    /* Number (double) */
    TOKEN_SYMBOL, /* Symbol (string) */

    /* The rest don't use Token.val */
    TOKEN_LIST_OPEN,
    TOKEN_LIST_CLOSE,

    /* Indicates that the following expression should only be evaluated
     * explicitly. */
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

/* Is `c' a token separator? Used by token_store() and input_read() */
bool is_token_separator(char c);

/* Allocate and fill an array of Tokens from the input. Must be freed by the
 * caller. */
Token* tokens_scan(char* input);

/* Print all tokens in a TOKEN_EOF terminated array */
void tokens_print(Token* arr);

#endif /* LEXER_H_ */
