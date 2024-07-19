
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/util.h"
#include "include/parser.h"
#include "include/expr.h"
#include "include/eval.h"
#include "include/primitives.h"
#include "include/env.h"

/*----------------------------------------------------------------------------*/
/* NOTE: Make sure we only allocate when we are sure the expression will be
 * valid, or we would need to free our allocations before returning NULL in case
 * of errors (e.g. when asserts fail) */

/* Evaluate each sub-expression as an argument for a procedure */
static Expr* eval_args(Env* env, Expr* list) {
    Expr* copy_start = NULL;
    Expr* cur_copy   = NULL;
    for (Expr* cur_item = list; cur_item != NULL; cur_item = cur_item->next) {
        if (copy_start == NULL) {
            /* Evaluate original argument, save it in our copy */
            cur_copy = eval(env, cur_item);

            /* If we haven't saved the first item of the linked list, save it */
            copy_start = cur_copy;
        } else {
            /* If it's not the first copy, keep filling the linked list */
            cur_copy->next = eval(env, cur_item);

            /* Move to the next argument in our copy list */
            cur_copy = cur_copy->next;
        }

        /* Failed to evaluate an item. Only `list' can be NULL. Stop. */
        if (cur_copy == NULL) {
            expr_free(copy_start);
            return NULL;
        }
    }

    return copy_start;
}

/* Is this expression a quote in the form (quote symbol)? */
static bool is_quote(const Expr* e) {
    /*
     * (define is-quote (e)
     *   (equal (car e) 'quote))
     */
    return e != NULL && e->type == EXPR_PARENT && e->val.children != NULL &&
           e->val.children->type == EXPR_SYMBOL &&
           e->val.children->val.s != NULL &&
           !strcmp(e->val.children->val.s, "quote");
}

Expr* eval(Env* env, Expr* e) {
    if (e == NULL)
        return NULL;

    /* Quoted expression in the form (quote symbol), pass `cadr' to primitive
     * without evaluating it. The primitive will just return a copy. */
    if (is_quote(e))
        return prim_quote(env, e->val.children->next);

    switch (e->type) {
        case EXPR_PARENT: {
            /* The evaluated car of the expression will be the function */
            Expr* func = e->val.children;

            /* We got a list with no children: NIL */
            if (func == NULL)
                return expr_clone(e);

            /* The cdr of the expression are the arguments, if any. They will be
             * evaluated by `apply', closing the eval-apply cycle. */
            Expr* args = func->next;

            /* Evaluate the function expression */
            func = eval(env, func);

            /* Could not eval function symbol/closure, stop */
            if (func == NULL)
                return NULL;

            /* Evaluate each of the arguments before calling primitive. Note
             * that the returned list is allocated, so we will have to free it
             * after calling `apply'. */
            args = eval_args(env, args);

            /* Apply expression as a function call */
            Expr* applied = apply(env, func, args);

            /* The last evaluations of `func' and `args' returned a clone, we
             * have to free it here. */
            expr_free(func);
            expr_free(args);

            return applied;
        }

        case EXPR_SYMBOL: {
            Expr* val = env_get(env, e->val.s);
            SL_ASSERT(val != NULL, "Unbound symbol: %s", e->val.s);
            return val;
        }

        case EXPR_ERR:
        case EXPR_CONST:
        case EXPR_PRIM: {
            /* Not a parent nor a symbol, evaluates to itself */
            return expr_clone(e);
        }
    }

    ERR("Reached unexpected point, didn't return from switch.");
    return NULL;
}

Expr* apply(Env* env, Expr* func, Expr* args) {
    SL_ASSERT(env != NULL, "Invalid environment.");
    SL_ASSERT(func != NULL, "Invalid function.");

    /* TODO: Add procedures, closures, etc. */
    SL_ASSERT(func->type == EXPR_PRIM,
              "Non-primitive functions are not supported for now.");

    /* Get primitive C function from the expression */
    PrimitiveFuncPtr primitive = func->val.f;
    SL_ASSERT(primitive != NULL, "Invalid function pointer.");

    /* Call primitive C function with the evaluated arguments we got from
     * `eval'. */
    return primitive(env, args);
}
