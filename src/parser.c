
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/util.h"
#include "include/lexer.h"
#include "include/expr.h"
#include "include/parser.h"

Expr* parse(Token* tokens) {
    Expr* root     = NULL;
    Expr* cur      = NULL;
    bool had_quote = false;

    for (int i = 0;; i++) {
        if (tokens[i].type == TOKEN_EOF || tokens[i].type == TOKEN_LIST_CLOSE) {
            /* NOTE: We don't allocate a NIL Expr, NULL indicates end of lists.
             * NOTE: Since TOKEN_LIST_CLOSE returns NULL, if Expr.children is
             *       NULL, it means it's a list with no children, i.e. NIL. */
            if (cur != NULL)
                cur->next = NULL;

            /* We reached EOF, we are done. */
            break;
        }

        if (tokens[i].type == TOKEN_QUOTE) {
            /* For quote tokens, just store it for the next token */
            had_quote = true;
            continue;
        }

        /* If we reached here, we should allocate an expression for this token
         * type. We will either start or add to the linked list. */
        if (root == NULL) {
            root = malloc(sizeof(Expr));
            cur  = root;
        } else {
            cur->next = malloc(sizeof(Expr));
            cur       = cur->next;
        }

        /* If the last token was TOKEN_QUOTE, mark it as quoted */
        cur->is_quoted = had_quote;
        had_quote      = false;

        switch (tokens[i].type) {
            case TOKEN_NUM:
                cur->type  = EXPR_CONST;
                cur->val.n = tokens[i].val.n;
                break;
            case TOKEN_SYMBOL:
                cur->type = EXPR_SYMBOL;

                /* Allocate a new copy of the string from tokens_scan(), since
                 * the lexer is responsible of freeing the original pointer. The
                 * ones we are allocating will be freed in expr_free() */
                cur->val.s = malloc(strlen(tokens[i].val.s));
                strcpy(cur->val.s, tokens[i].val.s);
                break;
            case TOKEN_LIST_OPEN:
                /* Parse starting from next token. See NIL note at the top. */
                cur->type         = EXPR_PARENT;
                cur->val.children = parse(&tokens[i + 1]);

                /* After parsing the nested list, skip those children tokens */
                int depth = 1;
                while (depth > 0) {
                    i++;
                    if (tokens[i].type == TOKEN_LIST_OPEN)
                        depth++;
                    else if (tokens[i].type == TOKEN_LIST_CLOSE)
                        depth--;
                }
                break;
            case TOKEN_EOF:
            case TOKEN_LIST_CLOSE:
            case TOKEN_QUOTE:
            default:
                ERR("Reached invalid case (Token type %d).", tokens[i].type);
                return NULL;
        }
    }

    return root;
}
