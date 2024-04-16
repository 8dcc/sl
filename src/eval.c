
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

Expr* eval(Env* env, Expr* e) {
    if (e == NULL)
        return NULL;

    /* Quoted expression, evaluates to itself */
    /* TODO: The user doesn't currently have a way of evaluating a quoted
     * expression explicitly. Add `eval' primitive. */
    if (e->is_quoted)
        return expr_clone_recur(e);

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

            /* Apply expression as function call */
            Expr* applied = apply(env, func, args);

            /* The last evaluation of `func' returned a clone, we have to free
             * it here. */
            expr_free(func);

            return applied;
        }
        case EXPR_SYMBOL: {
            Expr* val = env_get(env, e->val.s);
            SL_ASSERT(val != NULL, "Unbound symbol: %s", e->val.s);
            return val;
        }
        default:
            /* Not a parent nor a symbol, evaluates to itself */
            return expr_clone(e);
    }
}

Expr* apply(Env* env, Expr* func, Expr* args) {
    SL_ASSERT(env != NULL, "Invalid environment.");
    SL_ASSERT(func != NULL, "Invalid function.");

    /* TODO: Add closures, etc. */
    SL_ASSERT(func->type == EXPR_PRIM,
              "Non-primitive functions are not supported for now.");

    PrimitiveFuncPtr primitive = func->val.f;
    SL_ASSERT(primitive != NULL, "Invalid function pointer.");

    /* Evaluate each of the arguments before calling primitive. We will save the
     * evaluated expressions in another linked list to avoid double frees. */
    Expr* args_copy = NULL;
    Expr* cur_copy  = NULL;
    for (Expr* cur_arg = args; cur_arg != NULL; cur_arg = cur_arg->next) {
        if (args_copy == NULL) {
            /* Evaluate original argument, save it in our copy */
            cur_copy = eval(env, cur_arg);

            /* If we haven't saved the first item of the linked list, save it */
            args_copy = cur_copy;
        } else {
            /* If it's not the first copy, keep filling the linked list */
            cur_copy->next = eval(env, cur_arg);

            /* Move to the next argument in our copy list */
            cur_copy = cur_copy->next;
        }

        /* Failed to evaluate an argument. Only `args' can be NULL. Stop. */
        if (cur_copy == NULL) {
            expr_free(args_copy);
            return NULL;
        }
    }

    /* Finally, call primitive C function with the evaluated arguments */
    Expr* result = primitive(env, args_copy);

    /* Free the arguments we passed to primitive(), return only final Expr */
    expr_free(args_copy);

    return result;
}
