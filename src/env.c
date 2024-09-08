
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/expr.h"
#include "include/env.h"
#include "include/util.h"
#include "include/primitives.h"

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
    /*
     * The default environment has very little constants appart from C
     * primitives:
     *
     *   - nil: Empty list, used to represent "false". Parent expression with
     *     NULL as `val.children'.
     *   - tru: Symbol that evaluates to itself, used for explicit truth in
     *     boolean functions.
     *
     * Since the expressions will be cloned, it's safe to pass the stack address
     * to `env_bind'.
     */
    Expr nil_expr = {
        .type         = EXPR_PARENT,
        .val.children = NULL,
        .next         = NULL,
    };
    env_bind(env, "nil", &nil_expr);

    Expr tru_expr = {
        .type  = EXPR_SYMBOL,
        .val.s = "tru",
        .next  = NULL,
    };
    env_bind(env, "tru", &tru_expr);

    /* Bind primitive C functions */
    BIND_PRIM(env, "quote", quote);
    BIND_PRIM(env, "define", define);
    BIND_PRIM(env, "lambda", lambda);
    BIND_PRIM(env, "macro", macro);
    BIND_PRIM(env, "begin", begin);
    BIND_PRIM(env, "if", if);
    BIND_PRIM(env, "or", or);
    BIND_PRIM(env, "and", and);
    BIND_PRIM(env, "eval", eval);
    BIND_PRIM(env, "apply", apply);
    BIND_PRIM(env, "macroexpand", macroexpand);
    BIND_PRIM(env, "list", list);
    BIND_PRIM(env, "cons", cons);
    BIND_PRIM(env, "car", car);
    BIND_PRIM(env, "cdr", cdr);
    BIND_PRIM(env, "append", append);
    BIND_PRIM(env, "+", add);
    BIND_PRIM(env, "-", sub);
    BIND_PRIM(env, "*", mul);
    BIND_PRIM(env, "/", div);
    BIND_PRIM(env, "mod", mod);
    BIND_PRIM(env, "quotient", quotient);
    BIND_PRIM(env, "remainder", remainder);
    BIND_PRIM(env, "floor", floor);
    BIND_PRIM(env, "bit-and", bit_and);
    BIND_PRIM(env, "bit-or", bit_or);
    BIND_PRIM(env, "bit-xor", bit_xor);
    BIND_PRIM(env, "bit-not", bit_not);
    BIND_PRIM(env, "shr", shr);
    BIND_PRIM(env, "shl", shl);
    BIND_PRIM(env, "equal?", equal);
    BIND_PRIM(env, "<", lt);
    BIND_PRIM(env, ">", gt);
}

Env* env_clone(Env* env) {
    /*
     * When cloning an environment, the same parent pointer is shared, not a
     * copy. New arrays are allocated for symbols and values, and a new copy is
     * created for each one.
     */
    Env* cloned    = env_new();
    cloned->parent = env->parent;

    cloned->size    = env->size;
    cloned->symbols = sl_safe_malloc(cloned->size * sizeof(char*));
    cloned->values  = sl_safe_malloc(cloned->size * sizeof(Expr*));

    for (size_t i = 0; i < cloned->size; i++) {
        cloned->symbols[i] = sl_safe_strdup(env->symbols[i]);
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

    /*
     * Before creating a new item in the environment, traverse the existing
     * nodes on the linked list and check if one of the symbols matches what we
     * are trying to bind. If we find a match, overwrite its value.
     *
     * Otherwise, reallocate the `symbols' and `values' arrays, and add a clone
     * of the symbol string and value expression we received.
     */
    for (size_t i = 0; i < env->size; i++) {
        if (!strcmp(env->symbols[i], sym)) {
            expr_free(env->values[i]);
            env->values[i] = expr_clone_recur(val);
            return;
        }
    }

    env->size++;
    sl_safe_realloc(env->symbols, env->size * sizeof(char*));
    sl_safe_realloc(env->values, env->size * sizeof(Expr*));

    env->symbols[env->size - 1] = sl_safe_strdup(sym);
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
