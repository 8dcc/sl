
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/util.h"
#include "include/lexer.h"
#include "include/expr.h"
#include "include/parser.h"

static Expr* parse_recur(const Token* token, size_t* parsed) {
    SL_ASSERT(token != NULL, "Got invalid Token pointer.");
    SL_ASSERT(token->type != TOKEN_LIST_CLOSE, "Got invalid Token type.");

    if (token->type == TOKEN_EOF)
        return NULL;

    /* Number of parsed tokens in the current call, including recursive
     * sub-calls. See the comment on `parse' for more information. */
    *parsed = 1;

    Expr* expr = sl_safe_malloc(sizeof(Expr));
    expr->next = NULL;

    /* Expression type and value should be set on each case */
    switch (token->type) {
        case TOKEN_NUM: {
            expr->type  = EXPR_CONST;
            expr->val.n = token->val.n;
        } break;

        case TOKEN_SYMBOL: {
            expr->type = EXPR_SYMBOL;

            /* Allocate a new copy of the string from `tokens_scan', since the
             * lexer is responsible of freeing the original pointer. The ones we
             * are allocating will be freed in `expr_free'. */
            expr->val.s = strdup(token->val.s);
        } break;

        case TOKEN_LIST_OPEN: {
            expr->type = EXPR_PARENT;

            /* dummy.next will contain the first children of the list */
            Expr dummy;
            dummy.next = NULL;

            /* Parse each children */
            Expr* cur_child = &dummy;
            while (token[*parsed].type != TOKEN_LIST_CLOSE &&
                   token[*parsed].type != TOKEN_EOF) {
                SL_ASSERT(cur_child != NULL, "Unexpected NULL child.");

                /* Parse the current children recursively, storing the parsed
                 * Tokens in that call. */
                size_t parsed_in_call = 0;
                cur_child->next = parse_recur(&token[*parsed], &parsed_in_call);
                cur_child       = cur_child->next;

                SL_ASSERT(parsed_in_call > 0,
                          "When iterating a list, no tokens were parsed after "
                          "a recursive call.");

                /* Add the number of Tokens parsed in the previous call to the
                 * number of parsed in the current one. */
                *parsed += parsed_in_call;
            }

            /* Store that we also parsed the LIST_CLOSE or EOF Token */
            *parsed += 1;

            /* Store the start of the linked list in the parent expression */
            expr->val.children = dummy.next;
        } break;

        case TOKEN_QUOTE: {
            /* If the last Token was TOKEN_QUOTE, convert to (quote ...) */
            expr->type = EXPR_PARENT;

            /* First element of the list, "quote" */
            expr->val.children        = expr_new(EXPR_SYMBOL);
            expr->val.children->val.s = strdup("quote");

            /* Second element, the actual expression. */
            size_t parsed_in_call = 0;
            expr->val.children->next =
              parse_recur(&token[*parsed], &parsed_in_call);

            SL_ASSERT(parsed_in_call > 0, "When parsing a quoted expression, "
                                          "no tokens were parsed.");

            /* Add the number of Tokens parsed in the previous call to the
             * number of parsed in the current one. */
            *parsed += parsed_in_call;
        } break;

        case TOKEN_EOF:
        case TOKEN_LIST_CLOSE: {
            ERR("Reached invalid case (Token type %d).", token->type);
            abort();
        }
    }

    return expr;
}

Expr* parse(const Token* token) {
    /*
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
    size_t unused;
    return parse_recur(token, &unused);
}
