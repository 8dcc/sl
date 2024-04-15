
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

Expr* eval_expr(Env* env, Expr* e) {
    if (e == NULL)
        return NULL;

    /* Quoted expression, evaluates to itself */
    /* TODO: The user doesn't currently have a way of evaluating a quoted
     * expression explicitly. Add `eval' primitive. */
    if (e->is_quoted)
        return expr_clone_recur(e);

    switch (e->type) {
        case EXPR_PARENT:
            /* Evaluate S-Expression as a function call */
            return eval_sexpr(env, e);
        case EXPR_SYMBOL:
            Expr* val = env_get(env, e->val.s);
            SL_ASSERT(val != NULL, "Unknown LISP symbol: %s", e->val.s);
            return val;
        default:
            /* Not a parent nor a symbol, evaluates to itself */
            return expr_clone(e);
    }
}

Expr* eval_sexpr(Env* env, Expr* e) {
    if (e == NULL)
        return NULL;

    /* It's a parent, get first children of the list */
    Expr* children = e->val.children;

    if (children == NULL)
        return expr_clone(e); /* NIL */

    /* First item of the list must be the function name */
    SL_ASSERT(children->type == EXPR_SYMBOL, "Function name is of type: %s",
              exprtype2str(children->type));
    SL_ASSERT(children->val.s != NULL,
              "Function name is a symbol, but has no value.");

    /* Now we need to get the function pointer corresponding to the specified
     * symbol.
     *
     * First, we will check for special primitives like `quote', whose arguments
     * should not be evaluated automatically. */
    for (int i = 0; non_eval_primitives[i].s != NULL; i++)
        if (!strcmp(children->val.s, non_eval_primitives[i].s))
            return primitives[i].f(children->next);

    /* Then, we should check for normal C primitives. */
    PrimitiveFuncPtr primitive = NULL;
    for (int i = 0; primitives[i].s != NULL; i++) {
        if (!strcmp(children->val.s, primitives[i].s)) {
            primitive = primitives[i].f;
            break;
        }
    }

    /* TODO: Look for symbol in `env' */
    SL_ASSERT(primitive != NULL, "Function \"%s\" not found.", children->val.s);

    /* Evaluate each of the arguments before calling primitive. We will save the
     * evaluated expressions in another linked list to avoid double frees. */
    Expr* first_arg = NULL;
    Expr* arg_copy  = NULL;
    for (Expr* arg_orig = children->next; arg_orig != NULL;
         arg_orig       = arg_orig->next) {
        if (first_arg == NULL) {
            /* Evaluate original argument, save it in our copy */
            arg_copy = eval_expr(env, arg_orig);

            /* If we haven't saved the first item of the linked list, save it */
            first_arg = arg_copy;
        } else {
            /* If it's not the first copy, keep filling the linked list */
            arg_copy->next = eval_expr(env, arg_orig);

            /* Move to the next argument in our copy list */
            arg_copy = arg_copy->next;
        }
    }

    /* Finally, call function with the evaluated arguments */
    Expr* result = primitive(first_arg);

    /* Free the arguments we passed to primitive(), return only final Expr */
    expr_free(first_arg);

    return result;
}
