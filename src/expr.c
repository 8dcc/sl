
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
        case EXPR_ERR:
        case EXPR_CONST:
        case EXPR_PRIM:
            break;
    }

    /* Free the expression itself, that we allocated on parse() */
    free(root);
}

/*----------------------------------------------------------------------------*/

#define INDENT_STEP 4
void expr_print_debug(Expr* e) {
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
        case EXPR_PARENT:
            printf("[LST]");

            /* List with no children: NIL */
            if (e->val.children == NULL) {
                printf(" (NIL)\n");
                break;
            }

            putchar('\n');

            /* If the token is a parent, indent and print all children */
            indent += INDENT_STEP;
            expr_print_debug(e->val.children);
            indent -= INDENT_STEP;
            break;
        case EXPR_PRIM:
            printf("[PRI] <primitive %p>\n", e->val.f);
            break;
        case EXPR_ERR:
            printf("[ERR] (Stopping)");
            return;
    }

    if (e->next != NULL)
        expr_print_debug(e->next);
}

static void print_sexpr(Expr* e) {
    putchar('(');
    for (Expr* cur = e->val.children; cur != NULL; cur = cur->next) {
        expr_print(cur);

        if (cur->next != NULL)
            putchar(' ');
    }
    putchar(')');
}

void expr_print(Expr* e) {
    /* This function shouldn't be called with NULL */
    if (e == NULL) {
        ERR("Got null expression.");
        return;
    }

    switch (e->type) {
        case EXPR_CONST:
            printf("%f", e->val.n);
            break;
        case EXPR_SYMBOL:
            printf("%s", e->val.s);
            break;
        case EXPR_PARENT:
            /* List with no children: NIL */
            if (e->val.children == NULL)
                printf("nil");
            else
                print_sexpr(e);

            break;
        case EXPR_PRIM:
            printf("<primitive %p>", e->val.f);
            break;
        case EXPR_ERR:
            printf("<error>");
            break;
    }
}

void expr_println(Expr* e) {
    expr_print(e);
    putchar('\n');
}

/*----------------------------------------------------------------------------*/

Expr* expr_clone(const Expr* e) {
    if (e == NULL)
        return NULL;

    Expr* ret = sl_safe_malloc(sizeof(Expr));
    ret->type = e->type;

    switch (e->type) {
        case EXPR_SYMBOL:
            /* Allocate a new copy of the string */
            ret->val.s = strdup(e->val.s);
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
        case EXPR_ERR:
            ERR("Trying to clone <error>");
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
