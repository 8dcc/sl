
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/expr.h"

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
        default:
        case EXPR_PARENT:
            ret->val.children = NULL;
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
