
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/util.h"
#include "include/parser.h"
#include "include/expr.h"
#include "include/eval.h"
#include "include/primitives.h"

typedef Expr* (*FuncPtr)(Expr*);

typedef struct {
    const char* s;
    FuncPtr f;
} SymbolFuncPair;

static SymbolFuncPair primitives[] = {
    { "+", prim_add },
    { "-", prim_sub },
    { "*", prim_mul },
    { "/", prim_div },
};

/*----------------------------------------------------------------------------*/
/* NOTE: Make sure we only allocate when we are sure the expression will be
 * valid, or we would need to free our allocations before returning NULL in case
 * of errors (e.g. when asserts fail) */

Expr* eval_expr(Expr* e) {
    if (e == NULL)
        return NULL;

    /* Quoted expression, evaluates to itself */
    /* TODO: The user doesn't currently have a way of evaluating a quoted
     * expression explicitly. */
    if (e->is_quoted)
        return expr_clone(e);

    if (e->type == EXPR_PARENT) {
        /* Evaluate S-Expression as a function call */
        return eval_sexpr(e);
    } else {
        /* Not a parent, evaluates to itself */
        return expr_clone(e);
    }
}

Expr* eval_sexpr(Expr* e) {
    if (e == NULL)
        return NULL;

    /* It's a parent, get first children of the list */
    Expr* children = e->val.children;

    if (children == NULL)
        return expr_clone(e); /* NIL */

    /* First item of the list must be the function name */
    SL_ASSERT(children->type == EXPR_SYMBOL,
              "Trying to call function without a name.");
    SL_ASSERT(children->val.s != NULL,
              "Function name is a symbol, but has no value.");

    /* Handle special functions/macros like `quote' */
    if (!strcmp(children->val.s, "quote")) {
        /* We have to use `expr_clone_recur' since `expr_clone' does not clone
         * children. */
        Expr* cloned      = expr_clone_recur(children->next);
        cloned->is_quoted = true;
        return cloned;
    }

    /* Get the function pointer corresponding to the function */
    FuncPtr sym_func = NULL;
    for (int i = 0; i < LENGTH(primitives); i++) {
        if (!strcmp(children->val.s, primitives[i].s)) {
            sym_func = primitives[i].f;
            break;
        }
    }

    SL_ASSERT(sym_func != NULL, "Function \"%s\" not found.", children->val.s);

    /* Evaluate each of the arguments before calling primitive. We will save the
     * evaluated expressions in another linked list to avoid double frees. */
    Expr* first_arg = NULL;
    Expr* arg_copy  = NULL;
    for (Expr* arg_orig = children->next; arg_orig != NULL;
         arg_orig       = arg_orig->next) {
        if (first_arg == NULL) {
            /* Evaluate original argument, save it in our copy */
            arg_copy = eval_expr(arg_orig);

            /* If we haven't saved the first item of the linked list, save it */
            first_arg = arg_copy;
        } else {
            /* If it's not the first copy, keep filling the linked list */
            arg_copy->next = eval_expr(arg_orig);

            /* Move to the next argument in our copy list */
            arg_copy = arg_copy->next;
        }
    }

    /* Finally, call function with the evaluated arguments */
    Expr* result = sym_func(first_arg);

    /* Free the arguments we passed to sym_func(), return only final Expr */
    expr_free(first_arg);

    return result;
}
