
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/util.h"
#include "include/lexer.h"
#include "include/expr.h"
#include "include/parser.h"

static Expr* parse_expr(Token** token_ptr) {
    const Token* token = *token_ptr;

    /* Close expression */
    if (token->type == TOKEN_EOF || token->type == TOKEN_LIST_CLOSE)
        return NULL;

    /* We are parsing a new expression */
    Expr* expr = malloc(sizeof(Expr));
    SL_ASSERT_ALLOC(expr);

    /* Set expr->type and expr->val depending on the Token type */
    switch (token->type) {
        case TOKEN_NUM:
            expr->type  = EXPR_CONST;
            expr->val.n = token->val.n;
            break;
        case TOKEN_SYMBOL:
            expr->type = EXPR_SYMBOL;

            /* Allocate a new copy of the string from tokens_scan(), since
             * the lexer is responsible of freeing the original pointer. The
             * ones we are allocating will be freed in expr_free() */
            expr->val.s = strdup(token->val.s);
            break;
        case TOKEN_LIST_OPEN:
            expr->type = EXPR_PARENT;

            /* Parse first expression of the list. We change and pass
             * `token_ptr' to let the caller know how much we parsed. */
            (*token_ptr)++;
            expr->val.children = parse_expr(token_ptr);

            for (Expr* cur_expr = expr->val.children;;
                 cur_expr       = cur_expr->next) {
                /* Go to the next Token and parse recursively */
                (*token_ptr)++;
                cur_expr->next = parse_expr(token_ptr);

                /* We need to check after calling parse_expr() */
                if (cur_expr->next == NULL)
                    break;
            }
            break;
        case TOKEN_QUOTE:
            /* If the last token was TOKEN_QUOTE, convert to (quote ...) */
            expr->type = EXPR_PARENT;

            /* Car of the list, "quote" */
            expr->val.children = malloc(sizeof(Expr));
            SL_ASSERT_ALLOC(expr->val.children);
            expr->val.children->type  = EXPR_SYMBOL;
            expr->val.children->val.s = strdup("quote");

            /* Cdr of the list, the actual expression. About `token_ptr', see
             * the LIST_OPEN case.*/
            (*token_ptr)++;
            expr->val.children->next = parse_expr(token_ptr);
            break;
        case TOKEN_EOF:
        case TOKEN_LIST_CLOSE:
            ERR("Reached invalid case (Token type %d).", token->type);
            return NULL;
    }

    /* If we were called recursively (e.g. when parsing lists), this will be
     * overwritten by the caller if needed. */
    expr->next = NULL;

    return expr;
}

Expr* parse(Token* token) {
    return parse_expr(&token);
}
