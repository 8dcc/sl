#ifndef LEXER_H_
#define LEXER_H_ 1

enum ETokenTypes {
    TOKEN_EOF,

    /* The following make use of Token.val */
    TOKEN_NUM, /* Number (double) */
    TOKEN_SYM, /* Symbol (string) */

    /* The rest don't use Token.val */
    TOKEN_LIST_OPEN,
    TOKEN_LIST_CLOSE,

    /* TODO: Quote, etc. */
};

typedef struct Token {
    enum ETokenTypes type;
    union {
        double n;
        char* s;
    } val;
} Token;

/* Allocate and fill an array of Tokens from the input. Must be freed by the
 * caller. */
Token* tokens_scan(char* input);

/* Print all tokens in a TOKEN_EOF terminated array */
void tokens_print(Token* arr);

#endif /* LEXER_H_ */
