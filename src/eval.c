
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/expr.h"
#include "include/env.h"
#include "include/lambda.h"
#include "include/util.h"
#include "include/eval.h"
#include "include/primitives.h"

/*----------------------------------------------------------------------------*/
/* NOTE: Make sure we only allocate when we are sure the expression will be
 * valid, or we would need to free our allocations before returning NULL in case
 * of errors (e.g. when asserts fail) */

/* Evaluate each sub-expression as an argument for a procedure */
static Expr* eval_args(Env* env, Expr* list) {
    /* The first item will be stored in dummy_copy.next */
    Expr dummy_copy;
    dummy_copy.next = NULL;
    Expr* cur_copy  = &dummy_copy;

    for (Expr* cur = list; cur != NULL; cur = cur->next) {
        /* Evaluate original argument, save it in our copy */
        cur_copy->next = eval(env, cur);

        /* Move to the next argument in our copy list */
        cur_copy = cur_copy->next;

        /* Failed to evaluate an item. Free what we had evaluated and stop. */
        if (cur_copy == NULL) {
            expr_free(dummy_copy.next);
            return NULL;
        }
    }

    return dummy_copy.next;
}

/* Does this expression match the form (NAME ...)? */
static bool is_special_form(const Expr* e, const char* name) {
    /*
     * (define is-quote (e)
     *   (and (not (null e))
     *        (equal (car e) 'quote)))
     */
    return e != NULL && e->type == EXPR_PARENT && e->val.children != NULL &&
           e->val.children->type == EXPR_SYMBOL &&
           e->val.children->val.s != NULL &&
           !strcmp(e->val.children->val.s, name);
}

Expr* eval(Env* env, Expr* e) {
    SL_ON_ERR(return NULL);

    if (e == NULL)
        return NULL;

    /*
     * Check for Special Form primitives (See SICP Chapter 4.1.1):
     *   - Quoted expression in the form (quote symbol), pass `cadr' to
     *     primitive without evaluating it. The primitive will just return a
     *     copy.
     *   - When defining a variable, the symbol should not be evaluated. The
     *     value, however, will be evaluated from the primitive (our Lisp don't
     *     use lazy evaluation).
     *   - The arguments of a call to `lambda' (formal arguments and body) are
     *     not supposed to be evaluated.
     */
    if (is_special_form(e, "quote"))
        return prim_quote(env, e->val.children->next);
    if (is_special_form(e, "define"))
        return prim_define(env, e->val.children->next);
    if (is_special_form(e, "lambda"))
        return prim_lambda(env, e->val.children->next);
    if (is_special_form(e, "if"))
        return prim_if(env, e->val.children->next);

    switch (e->type) {
        case EXPR_PARENT: {
            /* The evaluated car of the expression will be the function */
            Expr* func = e->val.children;

            /* We got a list with no children: NIL */
            if (func == NULL)
                return expr_clone(e);

            /* Store the original function arguments in a variable before
             * evaluating the function itself. Note that the cdr of the function
             * call is the argument list, if any. */
            Expr* args = func->next;

            /* Evaluate the function expression */
            func = eval(env, func);

            /* Could not eval function symbol/closure, stop */
            if (func == NULL)
                return NULL;

            /* Evaluate each of the arguments before applying them to the
             * function. Also note that the returned list is allocated, so we
             * will have to free it after calling `apply'. */
            args = eval_args(env, args);

            /* Apply the evaluated function to the evaluated argument list */
            Expr* applied = apply(env, func, args);

            /* The last evaluations of `func' and `args' returned a clone, we
             * have to free it here. */
            expr_free(func);
            expr_free(args);

            return applied;
        }

        case EXPR_SYMBOL: {
            Expr* val = env_get(env, e->val.s);
            SL_EXPECT(val != NULL, "Unbound symbol: %s", e->val.s);
            return val;
        }

        case EXPR_ERR:
        case EXPR_CONST:
        case EXPR_PRIM:
        case EXPR_LAMBDA: {
            /* Not a parent nor a symbol, evaluates to itself */
            return expr_clone(e);
        }
    }

    ERR("Reached unexpected point, didn't return from switch.");
    return NULL;
}

/*----------------------------------------------------------------------------*/

Expr* apply(Env* env, Expr* func, Expr* args) {
    SL_ON_ERR(return NULL);
    SL_EXPECT(env != NULL, "Invalid environment.");
    SL_EXPECT(func != NULL, "Invalid function.");

    switch (func->type) {
        case EXPR_PRIM: {
            /* Get primitive C function from the expression */
            PrimitiveFuncPtr primitive = func->val.prim;
            SL_ASSERT(primitive != NULL, "Invalid function pointer.");

            /* Call primitive C function with the evaluated arguments we got
             * from `eval'. */
            return primitive(env, args);
        }

        case EXPR_LAMBDA: {
            return lambda_call(env, func, args);
        }

        default: {
            ERR("Expected primitive or lambda, got '%s'.",
                exprtype2str(func->type));
            return NULL;
        }
    }
}
