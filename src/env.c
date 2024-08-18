
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/util.h"
#include "include/expr.h"
#include "include/primitives.h"
#include "include/env.h"

/* Used in `env_init_defaults' */
#define BIND_PRIM(ENV, SYM, FUNC)         \
    do {                                  \
        Expr FUNC##_expr = {              \
            .type     = EXPR_PRIM,        \
            .val.prim = prim_##FUNC,      \
            .next     = NULL,             \
        };                                \
        env_bind(ENV, SYM, &FUNC##_expr); \
    } while (0)

/*----------------------------------------------------------------------------*/

Env* env_new(void) {
    Env* env     = sl_safe_malloc(sizeof(Env));
    env->parent  = NULL;
    env->size    = 0;
    env->symbols = NULL;
    env->values  = NULL;
    return env;
}

void env_init_defaults(Env* env) {
    /* First constant of the environment, NIL. The Expr will be cloned, so it's
     * safe to pass the stack address to env_bind. */
    Expr nil_expr = {
        .type         = EXPR_PARENT,
        .val.children = NULL,
        .next         = NULL,
    };
    env_bind(env, "nil", &nil_expr);

    /* Bind primitive C functions */
    BIND_PRIM(env, "quote", quote);
    BIND_PRIM(env, "define", define);
    BIND_PRIM(env, "lambda", lambda);
    BIND_PRIM(env, "eval", eval);
    BIND_PRIM(env, "apply", apply);
    BIND_PRIM(env, "begin", begin);
    BIND_PRIM(env, "cons", cons);
    BIND_PRIM(env, "car", car);
    BIND_PRIM(env, "cdr", cdr);
    BIND_PRIM(env, "+", add);
    BIND_PRIM(env, "-", sub);
    BIND_PRIM(env, "*", mul);
    BIND_PRIM(env, "/", div);
}

Env* env_clone(Env* env) {
    Env* cloned = env_new();

    /* They share the same Env* for the parent, not a copy */
    cloned->parent = env->parent;

    /* Allocate the same number of symbols and values */
    cloned->size    = env->size;
    cloned->symbols = sl_safe_malloc(cloned->size * sizeof(char*));
    cloned->values  = sl_safe_malloc(cloned->size * sizeof(Expr*));

    /* Store a copy of each symbol and value pair */
    for (size_t i = 0; i < cloned->size; i++) {
        cloned->symbols[i] = strdup(env->symbols[i]);
        cloned->values[i]  = expr_clone_recur(env->values[i]);
    }

    return cloned;
}

void env_free(Env* env) {
    for (size_t i = 0; i < env->size; i++) {
        free(env->symbols[i]);
        expr_free(env->values[i]);
    }

    free(env->symbols);
    free(env->values);
    free(env);
}

/*----------------------------------------------------------------------------*/

/*
 * TODO: Add `env_*' function and primitive for binding a symbol to a value in
 * the global environment. Check how "define" works in Scheme and add a
 * def-global/def-local primitive as an alternative.
 */

void env_bind(Env* env, const char* sym, const Expr* val) {
    SL_ASSERT(env != NULL, "Invalid environment.");
    SL_ASSERT(sym != NULL, "Symbol is empty.");

    /* Iterate until the last node of the linked list */
    for (size_t i = 0; i < env->size; i++) {
        if (!strcmp(env->symbols[i], sym)) {
            /* We found a value associated to this symbol, overwrite with new
             * value. First we free the Expr we allocated on the first
             * asignment, and then we allocate a copy of the new value. */
            expr_free(env->values[i]);
            env->values[i] = expr_clone_recur(val);
            return;
        }
    }

    /* If we reached here, the symbol is not currently associated. Allocate one
     * more symbol and one more value in the environment. */
    env->size++;
    sl_safe_realloc(env->symbols, env->size * sizeof(char*));
    sl_safe_realloc(env->values, env->size * sizeof(Expr*));

    /* Copy the symbol name and clone the associated expression */
    env->symbols[env->size - 1] = strdup(sym);
    env->values[env->size - 1]  = expr_clone_recur(val);
}

Expr* env_get(Env* env, const char* sym) {
    SL_ASSERT(env != NULL, "Invalid environment.");
    SL_ASSERT(sym != NULL, "Symbol is empty.");

    /* Iterate the symbol list until we find the one we are looking for, then
     * return a copy of the value. */
    for (size_t i = 0; i < env->size; i++)
        if (!strcmp(env->symbols[i], sym))
            return expr_clone_recur(env->values[i]);

    /* We didn't find a value associated to that symbol. If there is a parent
     * environment, search in there. */
    return (env->parent == NULL) ? NULL : env_get(env->parent, sym);
}

/*----------------------------------------------------------------------------*/

void env_print(Env* env) {
    putchar('(');
    for (size_t i = 0; i < env->size; i++) {
        /* Add indentation to match parentheses of first line */
        if (i != 0)
            printf("\n ");

        printf("(\"%s\" ", env->symbols[i]);
        expr_print(env->values[i]);
        putchar(')');
    }
    printf(")\n");
}
