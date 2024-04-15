
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "include/util.h"
#include "include/expr.h"
#include "include/env.h"

void env_bind(Env** env, const char* sym, const Expr* val) {
    if (env == NULL) {
        ERR("Invalid environment pointer.");
        return;
    }

    if (sym == NULL) {
        ERR("Symbol is NULL.");
        return;
    }

    Env* new_node;
    if (*env == NULL) {
        /* Create the first node of the linked list */
        new_node = malloc(sizeof(Env));
        *env     = new_node;
    } else {
        /* Iterate until the last node of the linked list */
        Env* cur;
        for (cur = *env;; cur = cur->next) {
            if (!strcmp(cur->sym, sym)) {
                /* We found a value associated to this symbol, overwrite with
                 * new value. First we free the Expr we allocated on the first
                 * asignment, and then we allocate a copy of the new value. */
                expr_free(cur->val);
                cur->val = expr_clone_recur(val);
                return;
            }

            /* We need to check this after the strcmp() because we still want to
             * check if the last item contains our symbol. */
            if (cur->next == NULL)
                break;
        }

        /* If we reached here, the symbol is not currently associated. Create
         * new node and add it to the linked list. */
        new_node  = malloc(sizeof(Env));
        cur->next = new_node;
    }

    /* If we reached here, we are creating a new Env structure.
     * First, we copy the symbol name. */
    new_node->sym = malloc(strlen(sym));
    strcpy(new_node->sym, sym);

    /* Then, clone the expression associated to that symbol */
    new_node->val = expr_clone_recur(val);

    /* And lastly, we indicate that this is the new last item of the linked
     * list. */
    new_node->next = NULL;

    /* NOTE: We could return `new_node' here, if needed */
}

Expr* env_get(Env* env, const char* sym) {
    SL_ASSERT(sym != NULL, "Symbol is NULL.");

    /* For a more detailed explanation, see env_add() */
    for (Env* cur = env;; cur = cur->next) {
        /* We found a value associated to this symbol, return a copy */
        if (!strcmp(cur->sym, sym))
            return expr_clone_recur(cur->val);

        /* We reached the end of the list */
        if (cur->next == NULL)
            break;
    }

    /* We didn't find a value associated to that symbol */
    return NULL;
}

void env_init(Env** env) {
    /* NOTE: C primitives are handled separately. See eval.c */

    /* NIL */
    Expr nil_expr = {
        .type         = EXPR_PARENT,
        .val.children = NULL,
        .is_quoted    = false,
        .next         = NULL,
    };
    env_bind(env, "nil", &nil_expr);
}

void env_free(Env* env) {
    Env* cur = env;
    while (cur != NULL) {
        if (cur->sym != NULL)
            free(cur->sym);

        if (cur->val != NULL)
            expr_free(cur->val);

        Env* aux = cur->next;
        free(cur);
        cur = aux;
    }
}
