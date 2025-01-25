/*
 * Copyright 2024 8dcc
 *
 * This file is part of SL.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * SL. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef LEXER_H_
#define LEXER_H_ 1

#include <stdio.h> /* FILE */

#include "lisp_types.h" /* LispInt, LispFlt */

enum ETokenType {
    /*
     * Used to indicate the end of a 'Token' array.
     */
    TOKEN_EOF,

    /*
     * The following make use of 'Token.val'
     */
    TOKEN_NUM_INT, /* Number (LispInt) */
    TOKEN_NUM_FLT, /* Number (LispFlt) */
    TOKEN_SYMBOL,  /* Symbol (string) */
    TOKEN_STRING,  /* String (string) */

    /*
     * The rest don't make use of 'Token.val'.
     */
    TOKEN_LIST_OPEN,
    TOKEN_LIST_CLOSE,
    TOKEN_DOT,

    /*
     * Indicates that the next expression should be wrapped in (quote ...),
     * (` ...), (, ...) or (,@ ...) respectively.
     */
    TOKEN_QUOTE,
    TOKEN_BACKQUOTE,
    TOKEN_UNQUOTE,
    TOKEN_SPLICE,
};

typedef struct Token {
    enum ETokenType type;
    union {
        LispInt n;
        LispFlt f;
        char* s;
    } val;
} Token;

/*----------------------------------------------------------------------------*/

/*
 * Allocate and fill an array of Tokens from the input. Must be freed by the
 * caller.
 */
Token* tokenize(char* input);

/*
 * Free an array of tokens, along with all the memory used by each element.
 */
void tokens_free(Token* arr);

/*
 * Is 'c' a token separator? Used by 'get_token' and 'read_expr'.
 */
bool is_token_separator(char c);

/*
 * Print all tokens in a Token array terminated by TOKEN_EOF.
 */
void tokens_print(FILE* fp, Token* arr);

#endif /* LEXER_H_ */
