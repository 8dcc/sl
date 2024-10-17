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
#include "include/util.h"
#include "include/lexer.h"
#include "include/parser.h"

static size_t parse_recur(Expr* dst, const Token* tokens);

/*
 * Parse the next expression in `tokens[*parsed]', and wrap it in a list whose
 * first element is the symbol `func_name'.
 */
static size_t wrap_in_call(Expr* dst, const Token* tokens,
                           const char* func_name) {
    /* Create a list whose `car' is `func_name' */
    dst->type                = EXPR_PARENT;
    dst->val.children        = expr_new(EXPR_SYMBOL);
    dst->val.children->val.s = sl_safe_strdup(func_name);

    /*
     * The second element is the actual expression, which might consist of
     * multiple Tokens.
     */
    dst->val.children->next     = expr_new(EXPR_ERR);
    const size_t parsed_in_call = parse_recur(dst->val.children->next, tokens);

    SL_ASSERT(parsed_in_call > 0);
    return parsed_in_call;
}

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
            dst->val.s = sl_safe_strdup(tokens[0].val.s);
            parsed++;
        } break;

        case TOKEN_SYMBOL: {
            dst->type  = EXPR_SYMBOL;
            dst->val.s = sl_safe_strdup(tokens[0].val.s);
            parsed++;
        } break;

        case TOKEN_LIST_OPEN: {
            dst->type = EXPR_PARENT;
            parsed++;

            /* dummy.next will contain the first children of the list */
            Expr dummy;
            dummy.next      = NULL;
            Expr* cur_child = &dummy;

            while (tokens[parsed].type != TOKEN_LIST_CLOSE &&
                   tokens[parsed].type != TOKEN_EOF) {
                SL_ASSERT(cur_child != NULL);

                /*
                 * Parse the current children recursively, storing the parsed
                 * Tokens in that call.
                 */
                cur_child->next = expr_new(EXPR_ERR);
                const size_t parsed_in_call =
                  parse_recur(cur_child->next, &tokens[parsed]);
                cur_child = cur_child->next;

                /*
                 * Add the number of Tokens parsed in the previous call to the
                 * number of parsed in the current one.
                 */
                SL_ASSERT(parsed_in_call > 0);
                parsed += parsed_in_call;
            }

            /* Store that we also parsed the LIST_CLOSE or EOF Token */
            parsed += 1;

            /* Store the start of the linked list in the parent expression */
            dst->val.children = dummy.next;
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
     * FIXME: Update this comment.
     *
     * We need another function with another parameter because when it makes a
     * recursive call, the caller needs to know how many Tokens were parsed
     * recursively on that call (to skip over them).
     *
     * For example, the following string from `input_read':
     *
     *   '(+ 1 (- 3 2) 4)
     *
     * Would be converted into the following array of Tokens:
     *
     *   [ QUOTE, LIST_OPEN, "+", 1.000000, LIST_OPEN, "-", 3.000000, 2.000000,
     *     LIST_CLOSE, 4.000000, LIST_CLOSE, EOF ]
     *
     * When encountering QUOTE, we want to parse the next expression, but it
     * takes more than one Token. In this case, the expression next to the quote
     * ends on the last LIST_CLOSE Token. The recursive call to `parse_recur'
     * will use its second argument to let the caller know how many Tokens were
     * parsed, so it can continue where it needs.
     *
     * Same concept applies when parsing the inner list. We want to continue
     * parsing at the "4", not at the "3".
     */
    Expr* expr                 = expr_new(EXPR_ERR);
    const size_t tokens_parsed = parse_recur(expr, tokens);
    return (tokens_parsed == 0) ? NULL : expr;
}
