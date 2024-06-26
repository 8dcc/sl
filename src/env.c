
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/util.h"
#include "include/expr.h"
#include "include/primitives.h"
#include "include/env.h"

#define BIND_PRIM(TMP_ENV, SYM, FUNC) \
    Expr FUNC##_expr = {              \
        .type  = EXPR_PRIM,           \
        .val.f = prim_##FUNC,         \
        .next  = NULL,                \
    };                                \
    TMP_ENV = env_bind(TMP_ENV, SYM, &FUNC##_expr);

/*----------------------------------------------------------------------------*/

Env* env_bind(Env* env, const char* sym, const Expr* val) {
    SL_ASSERT(sym != NULL, "Symbol is empty.");

    Env* new_node;
    if (env == NULL) {
        /* Create the first node of the linked list */
        new_node = malloc(sizeof(Env));
        SL_ASSERT_ALLOC(new_node);

        env = new_node;
    } else {
        /* Iterate until the last node of the linked list */
        Env* cur;
        for (cur = env;; cur = cur->next) {
            if (!strcmp(cur->sym, sym)) {
                /* We found a value associated to this symbol, overwrite with
                 * new value. First we free the Expr we allocated on the first
                 * asignment, and then we allocate a copy of the new value. */
                expr_free(cur->val);
                cur->val = expr_clone_recur(val);
                return cur;
            }

            /* We need to check this after the strcmp() because we still want to
             * check if the last item contains our symbol. */
            if (cur->next == NULL)
                break;
        }

        /* If we reached here, the symbol is not currently associated. Create
         * new node and add it to the linked list. */
        new_node = malloc(sizeof(Env));
        SL_ASSERT_ALLOC(new_node);
        cur->next = new_node;
    }

    /* If we reached here, we are creating a new Env structure.
     * First, we copy the symbol name. */
    new_node->sym = strdup(sym);

    /* Then, clone the expression associated to that symbol */
    new_node->val = expr_clone_recur(val);

    /* And lastly, we indicate that this is the new last item of the linked
     * list. */
    new_node->next = NULL;

    /* Return `new_node' in case the caller needs it */
    return new_node;
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

    if (env == NULL) {
        ERR("Invalid environment pointer.");
        return;
    }

    /* NIL */
    Expr nil_expr = {
        .type         = EXPR_PARENT,
        .val.children = NULL,
        .next         = NULL,
    };

    if (*env == NULL) {
        /* Create the first node of the linked list. It will hold the `nil'
         * value. */
        *env = env_bind(NULL, "nil", &nil_expr);
    } else {
        /* We already have an enviroment, add it to the linked list. */
        env_bind(*env, "nil", &nil_expr);
        ERR("Re-initializing a non-null enviroment.");
    }

    Env* tmp_env = *env;
    BIND_PRIM(tmp_env, "quote", quote);
    BIND_PRIM(tmp_env, "define", define);
    BIND_PRIM(tmp_env, "eval", eval);
    BIND_PRIM(tmp_env, "apply", apply);
    BIND_PRIM(tmp_env, "cons", cons);
    BIND_PRIM(tmp_env, "car", car);
    BIND_PRIM(tmp_env, "cdr", cdr);
    BIND_PRIM(tmp_env, "+", add);
    BIND_PRIM(tmp_env, "-", sub);
    BIND_PRIM(tmp_env, "*", mul);
    BIND_PRIM(tmp_env, "/", div);
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

void env_print(Env* env) {
    putchar('(');
    for (Env* cur = env; cur != NULL; cur = cur->next) {
        /* If this isn't the first node, add indentation */
        if (cur != env)
            putchar(' ');

        printf("(\"%s\" ", cur->sym);
        expr_print(cur->val);
        putchar(')');

        /* If this isn't the last item, change line */
        if (cur->next != NULL)
            putchar('\n');
    }
    printf(")\n");
}
