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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/expr.h"
#include "include/env.h"
#include "include/expr_pool.h"
#include "include/util.h"
#include "include/memory.h"
#include "include/lexer.h"
#include "include/parser.h"

static size_t parse_recur(Expr* dst, const Token* tokens);

static inline bool is_list_closer(enum ETokenType token_type) {
    return token_type == TOKEN_LIST_CLOSE || token_type == TOKEN_EOF;
}

/*
 * Parse the next expression in 'tokens', and wrap it in a list whose first
 * element is the symbol 'func_name'. Return the number of parsed tokens.
 */
static size_t wrap_in_call(Expr* dst, const Token* tokens,
                           const char* func_name) {
    /*
     * First item of the list is the function name:
     *   (FUNC-NAME . ???)
     */
    dst->type       = EXPR_PAIR;
    CAR(dst)        = expr_new(EXPR_SYMBOL);
    CAR(dst)->val.s = mem_strdup(func_name);

    /*
     * The second element is the actual expression, which might consist of
     * multiple Tokens.
     *
     * The parsed expression will be placed in the `cadr' of the destination:
     *   (FUNC-NAME . (UNKNOWN . nil))
     */
    CDR(dst)  = expr_new(EXPR_PAIR);
    CADR(dst) = expr_new(EXPR_UNKNOWN);
    CDDR(dst) = g_nil;

    const size_t parsed_in_call = parse_recur(CADR(dst), tokens);
    SL_ASSERT(parsed_in_call > 0);
    return parsed_in_call;
}

/*
 * Parse an expression recursively. Writes to 'dst', and returns the number of
 * parsed tokens. See comment in 'parse' below.
 *
 * TODO: The following expressions fail assertions in the parser:
 *   sl> `,@
 * TODO: The following expressions should signal errors:
 *   sl> .
 *   sl> '(a . )
 */
static size_t parse_recur(Expr* dst, const Token* tokens) {
    SL_ASSERT(tokens != NULL);
    SL_ASSERT(tokens[0].type != TOKEN_LIST_CLOSE);

    if (tokens[0].type == TOKEN_EOF)
        return 0;

    size_t parsed = 0;

    /* Expression type and value should be set on each case */
    switch (tokens[0].type) {
        case TOKEN_NUM_INT: {
            dst->type  = EXPR_NUM_INT;
            dst->val.n = tokens[0].val.n;
            parsed++;
        } break;

        case TOKEN_NUM_FLT: {
            dst->type  = EXPR_NUM_FLT;
            dst->val.f = tokens[0].val.f;
            parsed++;
        } break;

        case TOKEN_STRING: {
            dst->type  = EXPR_STRING;
            dst->val.s = mem_strdup(tokens[0].val.s);
            parsed++;
        } break;

        case TOKEN_SYMBOL: {
            dst->type  = EXPR_SYMBOL;
            dst->val.s = mem_strdup(tokens[0].val.s);
            parsed++;
        } break;

        case TOKEN_LIST_OPEN: {
            parsed++;

            /*
             * Empty lists get replaced by the symbol "nil" in the parser.
             */
            if (is_list_closer(tokens[parsed].type)) {
                dst->type  = EXPR_SYMBOL;
                dst->val.s = mem_strdup("nil");
                parsed++;
                break;
            }

            /*
             * We got a non-empty list/pair, write the first 'car' and loop over
             * the rest of the list.
             */
            dst->type             = EXPR_PAIR;
            CAR(dst)              = expr_new(EXPR_UNKNOWN);
            size_t parsed_in_call = parse_recur(CAR(dst), &tokens[parsed]);
            CDR(dst)              = g_nil;
            SL_ASSERT(parsed_in_call > 0);
            parsed += parsed_in_call;

            Expr* cur = dst;
            while (!is_list_closer(tokens[parsed].type)) {
                /*
                 * If there is a dot inside the list, it indicates that the next
                 * element is the CDR, not the CAR of a new pair.
                 */
                if (tokens[parsed].type == TOKEN_DOT) {
                    parsed++;

                    /* TODO: Dot without a CDR value, propagate error upwards */
                    if (is_list_closer(tokens[parsed].type))
                        break;

                    CDR(cur)       = expr_new(EXPR_UNKNOWN);
                    parsed_in_call = parse_recur(CDR(cur), &tokens[parsed]);
                    SL_ASSERT(parsed_in_call > 0);
                    parsed += parsed_in_call;
                    break;
                }

                CDR(cur) = expr_new(EXPR_PAIR);
                cur      = CDR(cur);

                /*
                 * Parse the current children recursively, storing the parsed
                 * Tokens in that call.
                 */
                CAR(cur)       = expr_new(EXPR_UNKNOWN);
                parsed_in_call = parse_recur(CAR(cur), &tokens[parsed]);
                CDR(cur)       = g_nil;

                /*
                 * Add the number of Tokens parsed in the previous call to the
                 * number of parsed in the current one.
                 */
                SL_ASSERT(parsed_in_call > 0);
                parsed += parsed_in_call;
            }

            /* Store that we also parsed the LIST_CLOSE or EOF Token */
            parsed++;
        } break;

        case TOKEN_DOT: {
            /* TODO: Dot outside of a list, propagate error upwards */
        } break;

        case TOKEN_QUOTE: {
            /* Wrap the next expression in (quote ...) */
            parsed++;
            parsed += wrap_in_call(dst, &tokens[parsed], "quote");
        } break;

        case TOKEN_BACKQUOTE: {
            /* The function for backquoting is called "`". */
            parsed++;
            parsed += wrap_in_call(dst, &tokens[parsed], "`");
        } break;

        case TOKEN_UNQUOTE: {
            /* The function for unquoting is called ",". */
            parsed++;
            parsed += wrap_in_call(dst, &tokens[parsed], ",");
        } break;

        case TOKEN_SPLICE: {
            /* The function for splicing is called ",@". */
            parsed++;
            parsed += wrap_in_call(dst, &tokens[parsed], ",@");
        } break;

        case TOKEN_EOF:
        case TOKEN_LIST_CLOSE: {
            SL_FATAL("Reached invalid case (Token type %d).", tokens[0].type);
        }
    }

    return parsed;
}

Expr* parse(const Token* tokens) {
    /*
     * We need another function that writes to an 'Expr' parameter and that
     * returns the written bytes, since the function will call itself
     * recursively, and the caller must know how many elements of the 'Token'
     * array were parsed inside that recursive call (to skip over them).
     *
     * For example, if we received the following string from 'input_read':
     *
     *   (list '(a b c) 123)
     *
     * Would be converted into the following 'Token' array (as printed by
     * 'tokens_print'):
     *
     *   [ LIST_OPEN, "list", QUOTE, LIST_OPEN, "a", "b", "c", LIST_CLOSE, 123,
     *     LIST_CLOSE, EOF ]
     *
     * When encountering QUOTE, we want to parse the next expression, but it
     * takes more than one token (the whole list, 5 tokens). In this case, the
     * expression next to the quote ends on the first LIST_CLOSE token. The
     * recursive call to 'parse_recur' will use its return value to let the
     * caller know how many elements of the 'Token' array were parsed, so it can
     * continue where it needs (in this case, at the number 123).
     *
     * Same concept applies when parsing the list itself. After the recursive
     * call is done parsing the list, we want to continue parsing at the "123",
     * not the "a".
     */
    Expr* expr                 = expr_new(EXPR_UNKNOWN);
    const size_t tokens_parsed = parse_recur(expr, tokens);
    if (tokens_parsed == 0) {
        pool_free(expr);
        return NULL;
    }
    return expr;
}
