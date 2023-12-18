
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/util.h"
#include "include/lexer.h"
#include "include/parser.h"

/*
 * TODO:
 *   - "()" -> Just NIL instead of NIL inside LST
 */
Expr* parse(Token* tokens) {
    Expr* root = NULL;
    Expr* cur  = root;

    bool done = false;
    for (int i = 0; !done; i++) {
        if (tokens[i].type == TOKEN_EOF) {
            /* NOTE: We don't allocate a NIL Expr */
            cur->next = NULL;
            done      = true;
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
            case TOKEN_SYM:
                cur->type  = EXPR_SYMBOL;
                cur->val.s = tokens[i].val.s;
                break;
            case TOKEN_LIST_OPEN:
                cur->type         = EXPR_PARENT;
                cur->val.children = parse(&tokens[i + 1]);

                /* After parsing the nested list, skip the children tokens */
                int depth = 1;
                while (depth > 0) {
                    i++;
                    if (tokens[i].type == TOKEN_LIST_OPEN)
                        depth++;
                    else if (tokens[i].type == TOKEN_LIST_CLOSE)
                        depth--;
                }
                break;
            case TOKEN_LIST_CLOSE:
                /* Using a NIL token instead of just using `Expr.next = NULL'
                 * looks redundant, but it is its own Expr type. */
                cur->type = EXPR_NIL;
                cur->next = NULL;
                done      = true;
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
    /* TODO */
    (void)root;
}

#define INDENT_STEP 4
void expr_print(Expr* e) {
    static int indent = 0;

    for (int i = 0; i < indent; i++)
        putchar(' ');

    switch (e->type) {
        case EXPR_CONST:
            printf("[NUM] %f\n", e->val.n);
            break;
        case EXPR_SYMBOL:
            printf("[SYM] \"%s\"\n", e->val.s);
            break;
        case EXPR_PARENT:
            printf("[LST]\n");

            /* If the token is a parent, indent and print all children */
            indent += INDENT_STEP;
            expr_print(e->val.children);
            indent -= INDENT_STEP;
            break;
        case EXPR_NIL:
            printf("[NIL]\n");
            break;
        case EXPR_ERR:
        default:
            printf("[ERR]\n");
            ERR("Aborting...");
            return;
    }

    if (e->next != NULL)
        expr_print(e->next);
}
