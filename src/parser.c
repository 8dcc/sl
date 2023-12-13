
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/util.h"
#include "include/lexer.h"
#include "include/parser.h"

static int expr_count(Token* in) {
    int ret = 0;

    if (in[0].type != TOKEN_LIST_OPEN) {
        ERR("Expected list opening.");
        return 0;
    }

    for (int i = 1; in[i].type != TOKEN_LIST_CLOSE; i++) {
        ret++;

        /* Found nested list, skip children */
        if (in[i].type == TOKEN_LIST_OPEN) {
            int depth = 1;
            while (depth > 0) {
                i++;
                if (in[i].type == TOKEN_LIST_OPEN)
                    depth++;
                else if (in[i].type == TOKEN_LIST_CLOSE)
                    depth--;
            }
        }
    }

    return ret;
}

Expr* parse(Token* tokens) {
    if (tokens[0].type != TOKEN_LIST_OPEN) {
        /* TODO: If first token is a constant, parse it and return it. */
        ERR("Expected list opening.");
        return NULL;
    }

    Expr* root         = malloc(sizeof(Expr));
    root->type         = EXPR_PARENT;
    root->val.children = NULL;

    int children = expr_count(tokens);
    if (children <= 0)
        return NULL;

    /* DELME */
    printf("Children: %d\n", children);

    return root;
}

void expr_free(Expr* root) {
    /* TODO */
    (void)root;
}

void expr_print(Expr* root) {
    /* TODO */
    (void)root;
}
