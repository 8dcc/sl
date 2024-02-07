
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

Expr* eval(Expr* e) {
    if (e == NULL)
        return NULL;

    /* Not a parent, evaluates to itself */
    if (e->type != EXPR_PARENT)
        return e;

    /* It's a parent, get first children of the list */
    Expr* children = e->val.children;

    if (children == NULL)
        return e; /* NIL */

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

    /* TODO: This is probably leaking memory? What happens to old arguments
     * after they are evaluated? We can't just free because they might be in use
     * somewhere else.
     *
     * Perhaps a solution would be to return a copy of the expression when
     * checking the type, instead of returning that same pointer. That way the
     * original expression remains untouched and can be freed safely by the
     * caller.
     *
     * If we do that, maybe we should free the old pointer after evaluating? */

    /* Evaluate each of the arguments before calling primitive */
    for (Expr* i = children; i->next != NULL; i = i->next)
        i->next = eval(i->next);

    /* Finally, call function with the evaluated arguments */
    return sym_func(children->next);
}
