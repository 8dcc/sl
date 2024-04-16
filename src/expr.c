
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/expr.h"
#include "include/util.h"

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

/*----------------------------------------------------------------------------*/

/* TODO: Rename to expr_print_debug(), add cleaner expr_print() */
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
        case EXPR_PRIM:
            printf("[PRI] <primitive @ %p>", e->val.f);
            if (e->is_quoted)
                printf(" (QUOTE)");
            putchar('\n');
            break;
        case EXPR_ERR:
        default:
            printf("[ERR] ");
            ERR("Encountered invalid expression of type:",
                exprtype2str(e->type));
            return;
    }

    if (e->next != NULL)
        expr_print(e->next);
}

/*----------------------------------------------------------------------------*/

Expr* expr_clone(const Expr* e) {
    if (e == NULL)
        return NULL;

    Expr* ret      = malloc(sizeof(Expr));
    ret->type      = e->type;
    ret->is_quoted = e->is_quoted;

    switch (e->type) {
        case EXPR_SYMBOL:
            /* Allocate a new copy of the string, since the original will be
             * freed with the expression in expr_free(). */
            ret->val.s = malloc(strlen(e->val.s));
            strcpy(ret->val.s, e->val.s);
            break;
        case EXPR_CONST:
            ret->val.n = e->val.n;
            break;
        case EXPR_PARENT:
            ret->val.children = NULL;
            break;
        case EXPR_PRIM:
            ret->val.f = e->val.f;
            break;
        default:
            ret->val.children = NULL;
            ERR("Unhandled expression of type:", exprtype2str(e->type));
            break;
    }

    /* NOTE: We don't copy pointers like e->next or e->val.children  */
    ret->next = NULL;
    return ret;
}

Expr* expr_clone_recur(const Expr* e) {
    if (e == NULL)
        return NULL;

    Expr* cloned = expr_clone(e);

    /* If the expression we just cloned is a parent */
    if (e->type == EXPR_PARENT) {
        Expr* child_copy = NULL;

        /* Clone all children, while linking them together in the new list. This
         * is similar to how we evaluate function arguments in `eval' */
        for (Expr* cur_child = e->val.children; cur_child != NULL;
             cur_child       = cur_child->next) {
            if (cloned->val.children == NULL) {
                /* Clone the first children of the original */
                child_copy = expr_clone_recur(cur_child);

                /* Store that this was the first one for later */
                cloned->val.children = child_copy;
            } else {
                /* If we already cloned one, keep linking them */
                child_copy->next = expr_clone_recur(cur_child);

                /* Move to the next argument in our copy list */
                child_copy = child_copy->next;
            }
        }
    }

    return cloned;
}
