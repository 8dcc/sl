
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/util.h"
#include "include/lexer.h"
#include "include/expr.h"
#include "include/parser.h"

Expr* parse(Token* tokens) {
    static bool parse_single_expr = false;

    Expr* root = NULL;
    Expr* cur  = NULL;

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

        /* If we reached here, we should allocate an expression for this token
         * type. We will either start or add to the linked list. */
        if (root == NULL) {
            root = malloc(sizeof(Expr));
            cur  = root;
        } else {
            cur->next = malloc(sizeof(Expr));
            cur       = cur->next;
        }

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
            case TOKEN_QUOTE:
                /* If the last token was TOKEN_QUOTE, convert to (quote ...) */
                cur->type = EXPR_PARENT;

                /* Car of the list, "quote" */
                cur->val.children        = malloc(sizeof(Expr));
                cur->val.children->type  = EXPR_SYMBOL;
                cur->val.children->val.s = malloc(sizeof("quote"));
                strcpy(cur->val.children->val.s, "quote");

                /* Cdr of the list, the actual expression */
                parse_single_expr = true;
                /* FIXME: If the next parse() call parses a list, all the
                 * recursive calls will also be made with parse_single_expr as
                 * true. Just re-implement this whole Token* -> Expr* parser. */
                cur->val.children->next = parse(&tokens[i + 1]);
                parse_single_expr       = false;

                i++;
                if (tokens[i].type == TOKEN_LIST_OPEN) {
                    /* If the quoted expression we just parsed was a list, skip
                     * over it. It was an atom, just jumped over it before the
                     * conditional. */
                    int depth = 1;
                    while (depth > 0) {
                        i++;
                        if (tokens[i].type == TOKEN_LIST_OPEN)
                            depth++;
                        else if (tokens[i].type == TOKEN_LIST_CLOSE)
                            depth--;
                    }
                }
                break;
            case TOKEN_EOF:
            case TOKEN_LIST_CLOSE:
            default:
                ERR("Reached invalid case (Token type %d).", tokens[i].type);
                return NULL;
        }

        /* In this call, we only wanted to parse a single expression, for
         * example when quoting. See also first conditional of this loop. */
        if (parse_single_expr) {
            if (cur != NULL)
                cur->next = NULL;
            break;
        }
    }

    return root;
}
