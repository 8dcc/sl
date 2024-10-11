
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/expr.h"
#include "include/env.h"
#include "include/util.h"
#include "include/primitives.h"

/* Used in `env_init_defaults' */
#define BIND_PRIM(ENV, SYM, FUNC)                        \
    do {                                                 \
        Expr FUNC##_expr = {                             \
            .type     = EXPR_PRIM,                       \
            .val.prim = prim_##FUNC,                     \
            .next     = NULL,                            \
        };                                               \
        env_bind(ENV, SYM, &FUNC##_expr, ENV_FLAG_NONE); \
    } while (0)

/*----------------------------------------------------------------------------*/
/* Global constants */

static Expr nil_expr = {
    .type         = EXPR_PARENT,
    .val.children = NULL,
    .next         = NULL,
};

static Expr tru_expr = {
    .type  = EXPR_SYMBOL,
    .val.s = "tru",
    .next  = NULL,
};

const Expr* nil = &nil_expr;
const Expr* tru = &tru_expr;

/*----------------------------------------------------------------------------*/

Env* env_new(void) {
    Env* env      = sl_safe_malloc(sizeof(Env));
    env->parent   = NULL;
    env->size     = 0;
    env->bindings = NULL;
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
     *   - *debug-trace*: List of functions that are currently being traced by
     *     the debugger.
     *
     * Since the expressions will be cloned, it's safe to pass the stack address
     * to `env_bind'.
     */
    env_bind(env, "nil", nil, ENV_FLAG_CONST);
    env_bind(env, "tru", tru, ENV_FLAG_CONST);

    Expr debug_trace_list = {
        .type         = EXPR_PARENT,
        .val.children = NULL,
        .next         = NULL,
    };
    env_bind(env, "*debug-trace*", &debug_trace_list, ENV_FLAG_NONE);

    /* Bind primitive C functions */
    BIND_PRIM(env, "quote", quote);
    BIND_PRIM(env, "`", backquote);
    BIND_PRIM(env, "backquote", backquote);
    BIND_PRIM(env, ",", unquote);
    BIND_PRIM(env, ",@", splice);
    BIND_PRIM(env, "define", define);
    BIND_PRIM(env, "define-global", define_global);
    BIND_PRIM(env, "lambda", lambda);
    BIND_PRIM(env, "macro", macro);
    BIND_PRIM(env, "begin", begin);
    BIND_PRIM(env, "if", if);
    BIND_PRIM(env, "or", or);
    BIND_PRIM(env, "and", and);
    BIND_PRIM(env, "eval", eval);
    BIND_PRIM(env, "apply", apply);
    BIND_PRIM(env, "macroexpand", macroexpand);
    BIND_PRIM(env, "random", random);
    BIND_PRIM(env, "set-random-seed", set_random_seed);
    BIND_PRIM(env, "list", list);
    BIND_PRIM(env, "cons", cons);
    BIND_PRIM(env, "car", car);
    BIND_PRIM(env, "cdr", cdr);
    BIND_PRIM(env, "length", length);
    BIND_PRIM(env, "append", append);
    BIND_PRIM(env, "write-to-str", write_to_str);
    BIND_PRIM(env, "format", format);
    BIND_PRIM(env, "substring", substring);
    BIND_PRIM(env, "string-matches", string_matches);
    BIND_PRIM(env, "+", add);
    BIND_PRIM(env, "-", sub);
    BIND_PRIM(env, "*", mul);
    BIND_PRIM(env, "/", div);
    BIND_PRIM(env, "mod", mod);
    BIND_PRIM(env, "quotient", quotient);
    BIND_PRIM(env, "remainder", remainder);
    BIND_PRIM(env, "floor", floor);
    BIND_PRIM(env, "type-of", type_of);
    BIND_PRIM(env, "int?", is_int);
    BIND_PRIM(env, "flt?", is_flt);
    BIND_PRIM(env, "symbol?", is_symbol);
    BIND_PRIM(env, "string?", is_string);
    BIND_PRIM(env, "list?", is_list);
    BIND_PRIM(env, "primitive?", is_primitive);
    BIND_PRIM(env, "lambda?", is_lambda);
    BIND_PRIM(env, "macro?", is_macro);
    BIND_PRIM(env, "int->flt", int2flt);
    BIND_PRIM(env, "flt->int", flt2int);
    BIND_PRIM(env, "int->str", int2str);
    BIND_PRIM(env, "flt->str", flt2str);
    BIND_PRIM(env, "str->int", str2int);
    BIND_PRIM(env, "str->flt", str2flt);
    BIND_PRIM(env, "bit-and", bit_and);
    BIND_PRIM(env, "bit-or", bit_or);
    BIND_PRIM(env, "bit-xor", bit_xor);
    BIND_PRIM(env, "bit-not", bit_not);
    BIND_PRIM(env, "shr", shr);
    BIND_PRIM(env, "shl", shl);
    BIND_PRIM(env, "equal?", equal);
    BIND_PRIM(env, "=", equal_num);
    BIND_PRIM(env, "<", lt);
    BIND_PRIM(env, ">", gt);
    BIND_PRIM(env, "read", read);
    BIND_PRIM(env, "write", write);
    BIND_PRIM(env, "scan-str", scan_str);
    BIND_PRIM(env, "print-str", print_str);
    BIND_PRIM(env, "error", error);
}

Env* env_clone(Env* env) {
    /*
     * When cloning an environment, the same parent pointer is shared, not a
     * copy. New arrays are allocated for symbols and values, and a new copy is
     * created for each one.
     */
    Env* cloned    = env_new();
    cloned->parent = env->parent;

    cloned->size     = env->size;
    cloned->bindings = sl_safe_malloc(cloned->size * sizeof(EnvBinding));

    for (size_t i = 0; i < cloned->size; i++) {
        cloned->bindings[i].sym   = sl_safe_strdup(env->bindings[i].sym);
        cloned->bindings[i].val   = expr_clone_recur(env->bindings[i].val);
        cloned->bindings[i].flags = env->bindings[i].flags;
    }

    return cloned;
}

void env_free(Env* env) {
    for (size_t i = 0; i < env->size; i++) {
        free(env->bindings[i].sym);
        expr_free(env->bindings[i].val);
    }

    free(env->bindings);
    free(env);
}

/*----------------------------------------------------------------------------*/

bool env_bind(Env* env, const char* sym, const Expr* val,
              enum EEnvBindingFlag flags) {
    SL_ASSERT(env != NULL);
    SL_ASSERT(sym != NULL);
    SL_ON_ERR(return false);

    /*
     * Before creating a new item in the environment, traverse the existing
     * nodes on the linked list and check if one of the symbols matches what we
     * are trying to bind. If we find a match, and it's not a constant binding,
     * overwrite its value and flags.
     *
     * Otherwise, reallocate the `bindings' array, add a clone of the symbol
     * string, a clone of the value expression, and the flags we received.
     */
    for (size_t i = 0; i < env->size; i++) {
        if (strcmp(env->bindings[i].sym, sym) == 0) {
            SL_EXPECT((env->bindings[i].flags & ENV_FLAG_CONST) == 0,
                      "Trying to set constant symbol `%s'.", sym);

            expr_free(env->bindings[i].val);
            env->bindings[i].val   = expr_clone_recur(val);
            env->bindings[i].flags = flags;
            return true;
        }
    }

    env->size++;
    sl_safe_realloc(env->bindings, env->size * sizeof(EnvBinding));

    env->bindings[env->size - 1].sym   = sl_safe_strdup(sym);
    env->bindings[env->size - 1].val   = expr_clone_recur(val);
    env->bindings[env->size - 1].flags = flags;

    return true;
}

bool env_bind_global(Env* env, const char* sym, const Expr* val,
                     enum EEnvBindingFlag flags) {
    while (env->parent != NULL)
        env = env->parent;
    return env_bind(env, sym, val, flags);
}

Expr* env_get(const Env* env, const char* sym) {
    SL_ASSERT(env != NULL);
    SL_ASSERT(sym != NULL);

    /*
     * Iterate the symbol list until we find the one we are looking for, then
     * return a copy of the value.
     */
    for (size_t i = 0; i < env->size; i++)
        if (strcmp(env->bindings[i].sym, sym) == 0)
            return expr_clone_recur(env->bindings[i].val);

    /*
     * We didn't find a value associated to that symbol. If there is a parent
     * environment, search in there.
     */
    return (env->parent == NULL) ? NULL : env_get(env->parent, sym);
}

/*----------------------------------------------------------------------------*/

void env_print(FILE* fp, const Env* env) {
    fputc('(', fp);
    for (size_t i = 0; i < env->size; i++) {
        /* Add indentation to match parentheses of first line */
        if (i != 0)
            fprintf(fp, "\n ");

        /* Not the same order as the C structure, but prettier */
        fprintf(fp, "(%X \"%s\" ", env->bindings[i].flags,
                env->bindings[i].sym);
        expr_print(fp, env->bindings[i].val);
        fputc(')', fp);
    }
    fprintf(fp, ")\n");
}
