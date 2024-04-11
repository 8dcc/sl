
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/util.h"
#include "include/lexer.h"
#include "include/parser.h"

Expr* parse(Token* tokens) {
    Expr* root = NULL;
    Expr* cur  = NULL;

    bool done = false;
    for (int i = 0; !done; i++) {
        if (tokens[i].type == TOKEN_EOF || tokens[i].type == TOKEN_LIST_CLOSE) {
            /* NOTE: We don't allocate a NIL Expr, NULL indicates end of lists.
             * NOTE: Since TOKEN_LIST_CLOSE returns NULL, if Expr.children is
             *       NULL, it means it's a list with no children, i.e. NIL. */
            if (cur != NULL)
                cur->next = NULL;

            done = true;
            break;
        }

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
                cur->type  = EXPR_SYMBOL;
                cur->val.s = tokens[i].val.s;
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
            default:
                ERR("Should not reach the EOF case.");
                return NULL;
        }
    }

    return root;
}

void expr_free(Expr* root) {
    /* This function shouldn't be called with NULL */
    if (root == NULL)
        return;

    /* If the expression has an adjacent one, free that one first */
    if (root->next != NULL)
        expr_free(root->next);

    switch (root->type) {
        case EXPR_QUOTE:
        case EXPR_PARENT:
            /* If the expression has children, free them */
            if (root->val.children != NULL)
                expr_free(root->val.children);
            break;
        case EXPR_SYMBOL:
            /* Free the symbol string, allocated in tokens_scan() */
            free(root->val.s);
            break;
        default:
            break;
    }

    /* Free the expression itself, that we allocated on parse() */
    free(root);
}

#define INDENT_STEP 4
void expr_print(Expr* e) {
    static int indent = 0;

    /* This function shouldn't be called with NULL */
    if (e == NULL) {
        printf("[ERR] ");
        ERR("Got NULL as argument. Aborting...");
        return;
    }

    for (int i = 0; i < indent; i++)
        putchar(' ');

    switch (e->type) {
        case EXPR_CONST:
            printf("[NUM] %f\n", e->val.n);
            break;
        case EXPR_SYMBOL:
            printf("[SYM] \"%s\"\n", e->val.s);
            break;
        case EXPR_QUOTE:
        case EXPR_PARENT:
            printf(e->type == EXPR_QUOTE ? "[QTE]" : "[LST]");

            /* List with no children: NIL */
            if (e->val.children == NULL) {
                printf(" (NIL)\n");
                break;
            }

            putchar('\n');

            /* If the token is a parent, indent and print all children */
            indent += INDENT_STEP;
            expr_print(e->val.children);
            indent -= INDENT_STEP;
            break;
        case EXPR_ERR:
        default:
            printf("[ERR] ");
            ERR("Encountered invalid expression (type %d). Aborting...",
                e->type);
            return;
    }

    if (e->next != NULL)
        expr_print(e->next);
}
