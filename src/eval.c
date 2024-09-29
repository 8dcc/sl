
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

/*
 * NOTE: Make sure we only allocate when we are sure the expression will be
 * valid, or we would need to free our allocations before returning NULL in case
 * of errors (e.g. when asserts fail)
 */

/* Does this expression match the form "(NAME ...)"? */
static bool is_special_form(const Expr* e, const char* name) {
    /*
     * (defun is-special-form (e sym)
     *   (assert (and (list? e)
     *                (not (null? e))))
     *   (and (symbol? (car e))
     *        (equal? (car e) sym)))
     */
    SL_ASSERT(e != NULL && e->type == EXPR_PARENT && e->val.children != NULL);
    return e->val.children->type == EXPR_SYMBOL &&
           e->val.children->val.s != NULL &&
           strcmp(e->val.children->val.s, name) == 0;
}

/*
 * Evaluate each expression in a linked list by calling `eval', and return
 * another linked list with the results.
 */
static Expr* eval_list(Env* env, Expr* list) {
    /* The first item will be stored in `dummy_copy.next' */
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
            expr_list_free(dummy_copy.next);
            return NULL;
        }
    }

    return dummy_copy.next;
}

/*
 * Evaluate a list expression as a function call, applying the (evaluated) `car'
 * to the `cdr'. This function is responsible for evaluating the arguments
 * (using `eval_list') before applying the function, if necessary.
 */
static Expr* eval_function_call(Env* env, Expr* e) {
    SL_ON_ERR(return NULL);

#ifdef SL_DEBUG_TRACE
    static size_t trace_nesting = 0;

    for (size_t i = 0; i < trace_nesting; i++)
        printf("  ");
    printf("%zu: ", trace_nesting % 10);
    expr_print(e);
    putchar('\n');

    trace_nesting++;
#endif

    /* Caller should have checked if `e' is `nil' */
    SL_ASSERT(e->val.children != NULL);

    /* The `car' represents the function, and `cdr' represents the arguments */
    Expr* car = e->val.children;
    Expr* cdr = e->val.children->next;

    /*
     * Evaluate the expression representing the function. If the evaluation
     * fails, stop. Note that both the evaluated function and the evaluated list
     * of arguments is returned as an allocated clone, so we must free them when
     * we are done.
     */
    Expr* func = eval(env, car);
    if (func == NULL)
        return NULL;
    SL_EXPECT(expr_is_applicable(func), "Expected function or macro, got '%s'.",
              exprtype2str(func->type));

    /*
     * Normally, we should evaluate each of the arguments before applying the
     * function. However, we will skip this step if the function is a macro
     * because their arguments are not evaluated.  We will use this boolean when
     * evaluating and freeing.
     */
    const bool should_eval_args = (cdr != NULL && func->type != EXPR_MACRO);

    /*
     * If the arguments should be evaluated, evaluate them. If one of them
     * didn't evaluate correctly, an error message was printed so we just have
     * to stop.
     *
     * Otherwise, the arguments remain un-evaluated, but should not be freed.
     */
    Expr* args;
    if (should_eval_args) {
        args = eval_list(env, cdr);
        if (args == NULL) {
            expr_free(func);
            return NULL;
        }
    } else {
        args = cdr;
    }

    /* Apply the evaluated function to the evaluated argument list */
    Expr* applied = apply(env, func, args);

    /* The evaluations of `func' and `args' returned clones */
    expr_free(func);
    if (should_eval_args)
        expr_list_free(args);

#ifdef SL_DEBUG_TRACE
    trace_nesting--;

    for (size_t i = 0; i < trace_nesting; i++)
        printf("  ");
    printf("%zu: ", trace_nesting % 10);
    expr_print(applied);
    putchar('\n');
#endif

    return applied;
}

Expr* eval(Env* env, Expr* e) {
    SL_ON_ERR(return NULL);

    if (e == NULL)
        return NULL;

    switch (e->type) {
        case EXPR_PARENT: {
            /* `nil' evaluates to itself */
            if (expr_is_nil(e))
                return expr_clone(e);

            /*
             * If the list is a call to a Special Form, pass the un-evaluated
             * `cdr' to the primitive. See the "Special Forms" section of the SL
             * manual, and Chapter 4.1.1 of SICP.
             */
            if (is_special_form(e, "quote"))
                return prim_quote(env, e->val.children->next);
            if (is_special_form(e, "define"))
                return prim_define(env, e->val.children->next);
            if (is_special_form(e, "lambda"))
                return prim_lambda(env, e->val.children->next);
            if (is_special_form(e, "macro"))
                return prim_macro(env, e->val.children->next);
            if (is_special_form(e, "begin"))
                return prim_begin(env, e->val.children->next);
            if (is_special_form(e, "if"))
                return prim_if(env, e->val.children->next);
            if (is_special_form(e, "or"))
                return prim_or(env, e->val.children->next);
            if (is_special_form(e, "and"))
                return prim_and(env, e->val.children->next);

            /* Evaluate the list as a procedure/macro call */
            /* TODO: Replace most occurrences of "function" with "procedure" */
            return eval_function_call(env, e);
        }

        case EXPR_SYMBOL: {
            /* Symbols evaluate to the bound value in the current environment */
            Expr* val = env_get(env, e->val.s);
            SL_EXPECT(val != NULL, "Unbound symbol: %s", e->val.s);
            return val;
        }

        case EXPR_ERR:
        case EXPR_NUM_INT:
        case EXPR_NUM_FLT:
        case EXPR_STRING:
        case EXPR_PRIM:
        case EXPR_LAMBDA:
        case EXPR_MACRO: {
            /* Not a parent nor a symbol, evaluates to itself */
            return expr_clone(e);
        }
    }

    SL_FATAL("Reached unexpected point, didn't return from switch.");
}

/*----------------------------------------------------------------------------*/

Expr* apply(Env* env, Expr* func, Expr* args) {
    SL_ASSERT(env != NULL);
    SL_ASSERT(func != NULL);
    SL_ASSERT(expr_is_applicable(func));

    /*
     * Some important notes about the implementation of `apply':
     *   - It expects a valid environment and a valid applicable function (see
     *     the `expr_is_applicable' function in "expr.h").
     *   - The arguments, are expected to be evaluated by the caller whenever
     *     necessary. The arguments are passed to the function unchanged.
     *   - The `args' pointer can be NULL, since some functions expect no
     *     arguments. Again, the pointer is passed as-is.
     *   - Special forms are handled separately in `eval', so they can't be
     *     applied using this function.
     */
    Expr* result;

    switch (func->type) {
        case EXPR_PRIM: {
            /* Get primitive C function from the expression */
            PrimitiveFuncPtr primitive = func->val.prim;
            SL_ASSERT(primitive != NULL);

            /*
             * Call primitive C function with the evaluated arguments we got
             * from `eval'.
             */
            result = primitive(env, args);
        } break;

        case EXPR_LAMBDA: {
            /*
             * Call the lambda using the function defined in "lambda.c". A call
             * to a lambda is pretty straight-forward. Essentially you just have
             * to bind the value of each argument to its formal, and then
             * evaluate the body of the lambda.
             */
            result = lambda_call(env, func, args);
        } break;

        case EXPR_MACRO: {
            /*
             * Call the macro using the function defined in "lambda.c". A macro
             * receives some un-evaluated arguments and returns a list
             * representing an expression. When calling a macro, it is expanded
             * and the returned list is evaluated as an expression.
             */
            result = macro_call(env, func, args);
        } break;

        default: {
            SL_WRN("Expected 'Primitive' or 'Lambda', got '%s'.",
                   exprtype2str(func->type));
            result = NULL;
        } break;
    }

    return result;
}
