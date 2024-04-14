
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

void expr_free(Expr* root) {
    /* This function shouldn't be called with NULL */
    if (root == NULL)
        return;

    /* If the expression has an adjacent one, free that one first */
    if (root->next != NULL)
        expr_free(root->next);

    switch (root->type) {
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
            printf("[NUM] %f", e->val.n);
            if (e->is_quoted)
                printf(" (QUOTE)");
            putchar('\n');
            break;
        case EXPR_SYMBOL:
            printf("[SYM] \"%s\"", e->val.s);
            if (e->is_quoted)
                printf(" (QUOTE)");
            putchar('\n');
            break;
        case EXPR_PARENT:
            printf("[LST]");

            /* List with no children: NIL */
            if (e->val.children == NULL) {
                printf(" (NIL)");
                if (e->is_quoted)
                    printf(" (QUOTE)");
                putchar('\n');
                break;
            }

            if (e->is_quoted)
                printf(" (QUOTE)");
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
