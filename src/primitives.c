
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/util.h"
#include "include/parser.h"
#include "include/eval.h"
#include "include/expr.h"
#include "include/env.h"
#include "include/primitives.h"

#define EXPECT_ARG_NUM(EXPR, NUM)                            \
    SL_EXPECT(expr_list_len(EXPR) == NUM,                    \
              "Expected exactly %d arguments, got %d.", NUM, \
              expr_list_len(EXPR))

#define EXPECT_TYPE(EXPR, TYPE)                              \
    SL_EXPECT((EXPR)->type == TYPE,                          \
              "Expected expression of type '%s', got '%s'.", \
              exprtype2str(TYPE), exprtype2str((EXPR)->type))

/*----------------------------------------------------------------------------*/
/* Static util functions used by the primitives */

/* Count the number of mandatory and optional formal arguments in a list. */
static bool count_formals(const Expr* list, size_t* mandatory, size_t* optional,
                          bool* has_rest) {
    SL_ON_ERR(return false);

    /* Initialize output variables */
    *mandatory = 0;
    *optional  = 0;
    *has_rest  = false;

    /* Current stage when parsing the formal argument list */
    enum {
        READING_MANDATORY,
        READING_OPTIONAL,
        READING_REST,
    } state = READING_MANDATORY;

    for (const Expr* cur = list; cur != NULL; cur = cur->next) {
        SL_EXPECT(cur->type == EXPR_SYMBOL,
                  "Invalid formal argument expected type 'Symbol', got '%s'.",
                  exprtype2str(cur->type));

        if (strcmp(cur->val.s, "&optional") == 0) {
            /* Check if we should change state from mandatory to optional, and
             * continue reading. */
            SL_EXPECT(cur->next != NULL && state == READING_MANDATORY,
                      "Wrong usage of `&optional' keyword.");
            state = READING_OPTIONAL;
            continue;
        } else if (strcmp(cur->val.s, "&rest") == 0) {
            SL_EXPECT(cur->next != NULL && state != READING_REST,
                      "Wrong usage of `&rest' keyword.");
            state = READING_REST;
            continue;
        }

        switch (state) {
            case READING_MANDATORY:
                *mandatory += 1;
                break;
            case READING_OPTIONAL:
                *optional += 1;
                break;
            case READING_REST:
                /* Verify that the next argument to "&rest" is the last one. */
                SL_EXPECT(cur->next == NULL,
                          "Expected exactly 1 formal after `&rest' keyword.");
                *has_rest = true;
                break;
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/
/* Special Form primitives. See SICP Chapter 4.1.1 */

Expr* prim_quote(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL && e->next == NULL,
              "The special form `quote' expects exactly 1 argument.");

    /* This `prim' function is useful for binding it to `quote' in the
     * environment. */
    return expr_clone_recur(e);
}

Expr* prim_define(Env* env, Expr* e) {
    /*
     * The `define' function binds the even arguments to the odd arguments.
     * Therefore, it expects an even number of arguments.
     *
     * Returns an evaluated copy of the last bound expression.
     */
    Expr* last_bound = NULL;

    /* See note at the bottom */
    SL_ON_ERR(return last_bound);

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        SL_EXPECT(arg->next != NULL,
                  "Got an odd number of arguments, ignoring last.");

        /* Odd argument: Symbol */
        EXPECT_TYPE(arg, EXPR_SYMBOL);
        const char* sym = arg->val.s;

        /* Even argument: Expression */
        arg = arg->next;

        /* We evaluate the expression before binding. Invalid expressions are
         * not defined. */
        Expr* evaluated = eval(env, arg);
        if (evaluated == NULL)
            continue;

        /* Bind a copy of the evaluated expression to the current environment */
        env_bind(env, sym, evaluated);

        /* The `last_bound' variable holds either NULL or the last returned
         * expression by `eval'. Before overwriting the copy returned by `eval',
         * free it. */
        expr_free(last_bound);
        last_bound = evaluated;
    }

    /* Last bound holds either NULL or the last valid expression returned by
     * `eval'. Since eval returns a new expression, we can return it safely. */
    return last_bound;
}

Expr* prim_lambda(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL && e->next != NULL,
              "The special form `lambda' expects at least 2 arguments: Formals "
              "and body.");
    EXPECT_TYPE(e, EXPR_PARENT);

    /* Count and validate the formal arguments */
    size_t mandatory;
    size_t optional;
    bool has_rest;
    if (!count_formals(e->val.children, &mandatory, &optional, &has_rest))
        return NULL;
    const size_t total_formals = mandatory + optional + (has_rest ? 1 : 0);

    /*
     * Create a new LambdaCtx structure that will contain:
     *   - A new environment whose parent will be set when making the actual
     *     function call.
     *   - A string array for the formal arguments of the function, the first
     *     argument of `lambda'. It will be filled below.
     *   - The body of the function, the rest of the arguments of `lambda'.
     *
     * Note that since `lambda' is a special form, it's handled differently in
     * `eval' and its arguments won't be evaluated.
     */
    Expr* ret                = expr_new(EXPR_LAMBDA);
    ret->val.lambda          = sl_safe_malloc(sizeof(LambdaCtx));
    ret->val.lambda->env     = env_new();
    ret->val.lambda->formals = sl_safe_malloc(total_formals * sizeof(char*));
    ret->val.lambda->formals_mandatory = mandatory;
    ret->val.lambda->formals_optional  = optional;
    ret->val.lambda->formals_rest      = has_rest;
    ret->val.lambda->body              = expr_clone_list(e->next);

    /*
     * For each formal argument we counted above, store the symbol as a C string
     * in the array we just allocated. Note that we already verified that all of
     * the formals are symbols when counting them in `count_formals'.
     */
    Expr* cur_formal = e->val.children;
    size_t i         = 0;
    while (i < mandatory) {
        ret->val.lambda->formals[i++] = strdup(cur_formal->val.s);
        cur_formal                    = cur_formal->next;
    }

    /* Skip "&optional" */
    if (optional > 0)
        cur_formal = cur_formal->next;

    while (i < optional) {
        ret->val.lambda->formals[i++] = strdup(cur_formal->val.s);
        cur_formal                    = cur_formal->next;
    }

    if (has_rest) {
        /* Skip "&rest" */
        cur_formal = cur_formal->next;

        /* There should only be one formal left after "&rest" */
        ret->val.lambda->formals[i] = strdup(cur_formal->val.s);
    }

    return ret;
}

/*----------------------------------------------------------------------------*/
/* Primitives that should have their parameters evaluated by the caller */

Expr* prim_eval(Env* env, Expr* e) {
    return eval(env, e);
}

Expr* prim_apply(Env* env, Expr* e) {
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Got empty expression.");

    /*
     * (define prim-apply (env e)
     *   (apply (car e) (cdr e)))
     */
    return apply(env, e, e->next);
}

Expr* prim_begin(Env* env, Expr* e) {
    SL_ON_ERR(return NULL);

    /* In Scheme, begin is technically a special form because when making a
     * call, the arguments are not required to be evaluated in order. In this
     * Lisp, however, they are. */
    SL_EXPECT(e != NULL, "Expected at least 1 expression.");

    Expr* last_evaluated = NULL;
    for (Expr* cur = e; cur != NULL; cur = cur->next) {
        expr_free(last_evaluated);
        last_evaluated = eval(env, cur);
    }

    return last_evaluated;
}

Expr* prim_cons(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    EXPECT_ARG_NUM(e, 2);

    /*
     * The `cons' implementation is a bit different for now.
     *
     *   (cons x y) =!=> (x . y)
     *   (cons x y) ===> (x y)
     *
     * Maybe we could add a EXPR_CONS type.
     */
    Expr* ret         = expr_new(EXPR_PARENT);
    ret->val.children = expr_clone_recur(e);

    /* Append the cdr to the car */
    ret->val.children->next = expr_clone_recur(e->next);

    return ret;
}

Expr* prim_car(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    EXPECT_ARG_NUM(e, 1);
    EXPECT_TYPE(e, EXPR_PARENT);

    /* (car '()) ===> nil */
    if (e->val.children == NULL)
        return expr_clone(e);

    /*
     * (car '(a b c))     ===> a
     * (car '((a b) y z)) ===> (a b)
     */
    return expr_clone_recur(e->val.children);
}

Expr* prim_cdr(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    EXPECT_ARG_NUM(e, 1);
    EXPECT_TYPE(e, EXPR_PARENT);

    Expr* ret = expr_new(EXPR_PARENT);

    if (e->val.children == NULL || e->val.children->next == NULL) {
        /*
         * (cdr '())  ===> nil
         * (cdr '(a)) ===> nil
         */
        ret->val.children = NULL;
    } else {
        /*
         * (cdr '(a b c))     ===> (b c)
         * (cdr '((a b) y z)) ===> (y z)
         */
        ret->val.children = expr_clone_list(e->val.children->next);
    }

    return ret;
}

Expr* prim_add(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");

    double total = 0;

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        EXPECT_TYPE(arg, EXPR_CONST);

        total += arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_CONST);
    ret->val.n = total;
    return ret;
}

Expr* prim_sub(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");

    double total = e->val.n;

    /*
     * If there is only one argument, negate. Otherwise subtract in order.
     *   (- 5)     ===> -5
     *   (- 9 5 1) ===> 3
     */
    if (e->next == NULL) {
        total = -total;
    } else {
        for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
            EXPECT_TYPE(arg, EXPR_CONST);
            total -= arg->val.n;
        }
    }

    Expr* ret  = expr_new(EXPR_CONST);
    ret->val.n = total;
    return ret;
}

Expr* prim_mul(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");

    double total = 1;

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        EXPECT_TYPE(arg, EXPR_CONST);

        total *= arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_CONST);
    ret->val.n = total;
    return ret;
}

Expr* prim_div(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");

    double total = e->val.n;

    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        EXPECT_TYPE(arg, EXPR_CONST);
        SL_EXPECT(arg->val.n != 0, "Trying to divide by zero.");

        total /= arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_CONST);
    ret->val.n = total;
    return ret;
}
