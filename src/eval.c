
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/util.h"
#include "include/parser.h"
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

/* Allocate a new expr and copy `e'. Needed to avoid double frees and leaks. */
static Expr* expr_clone(Expr* e) {
    Expr* ret = malloc(sizeof(Expr));
    ret->type = e->type;

    switch (e->type) {
        case EXPR_SYMBOL:
            /* Allocate a new copy of the string, since the original will be
             * freed with the expression in expr_free(). */
            ret->val.s = malloc(strlen(e->val.s));
            strcpy(ret->val.s, e->val.s);
            break;
        case EXPR_CONST:
            ret->val.n = e->val.n;
            break;
        default:
        case EXPR_PARENT:
            e->val.children = NULL;
            break;
    }

    /* NOTE: We don't copy pointers like e->next or e->val.children  */
    ret->next = NULL;
    return ret;
}

Expr* eval(Expr* e) {
    if (e == NULL)
        return NULL;

    /* Not a parent, evaluates to itself */
    if (e->type != EXPR_PARENT)
        return expr_clone(e);

    /* It's a parent, get first children of the list */
    Expr* children = e->val.children;

    if (children == NULL)
        return expr_clone(e); /* NIL */

    /* First item of the list must be the function name */
    if (children->type != EXPR_SYMBOL) {
        ERR("Trying to call function without symbol.");
        return NULL;
    }

    if (children->val.s == NULL) {
        ERR("Function name is a symbol, but has no name.");
        return NULL;
    }

    /* Get the function pointer corresponding to the function */
    FuncPtr sym_func = NULL;
    for (int i = 0; i < LENGTH(primitives); i++) {
        if (!strcmp(children->val.s, primitives[i].s)) {
            sym_func = primitives[i].f;
            break;
        }
    }

    if (sym_func == NULL) {
        ERR("Function \"%s\" not found.", children->val.s);
        return NULL;
    }

    /* Evaluate each of the arguments before calling primitive. We will save the
     * evaluated expressions in another linked list to avoid double frees. */
    Expr* first_arg = NULL;
    Expr* arg_copy  = NULL;
    for (Expr* arg_orig = children->next; arg_orig != NULL;
         arg_orig       = arg_orig->next) {
        if (arg_copy == NULL) {
            /* Evaluate original argument, save it in our copy */
            arg_copy = eval(arg_orig);

            /* If we haven't saved the first item of the linked list, save it */
            first_arg = arg_copy;
        } else {
            /* If it's not the first copy, keep filling the linked list */
            arg_copy->next = eval(arg_orig);

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
