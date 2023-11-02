#ifndef LEXER_H_
#define LEXER_H_ 1

enum ETokenTypes {
    TOKEN_EOF,

    /* The following make use of Token.val */
    TOKEN_NUM, /* Integer */
    TOKEN_FLT, /* Float */
    TOKEN_SYM, /* Symbol (string) */

    /* The rest don't use Token.val */
    TOKEN_LIST_OPEN,
    TOKEN_LIST_CLOSE,

    /* TODO: Quote, etc. */
};

typedef struct Token {
    enum ETokenTypes type;
    union {
        int32_t i;
        float f;
        char* s;
    } val;
} Token;

/* Allocate and fill an array of Tokens from the input. Must be freed by the
 * caller. */
Token* scan_tokens(char* input);

#endif /* LEXER_H_ */
