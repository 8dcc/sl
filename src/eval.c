/*
 * Copyright 2024 8dcc
 *
 * This file is part of SL.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * SL. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/lambda.h"
#include "include/util.h"
#include "include/debug.h"
#include "include/eval.h"
#include "include/primitives.h"

/*
 * NOTE: Make sure we only allocate when we are sure the expression will be
 * valid, or we would need to free our allocations before returning NULL in case
 * of errors (e.g. when asserts fail)
 */

/*
 * Is this expression a special form symbol?
 */
static inline bool is_special_form(const Env* env, const Expr* e) {
    return EXPR_SYMBOL_P(e) && e->val.s != NULL &&
           (env_get_flags(env, e->val.s) & ENV_FLAG_SPECIAL) != 0;
}

/*
 * Evaluate each expression in a list by calling 'eval', and return another list
 * with the results. In Lisp jargon, map 'eval' to the specified list.
 *
 * TODO: We could rename this function to something like 'map_eval', or even add
 * a 'mapcar' C function that receives a 'PrimitiveFuncPtr'.
 */
static Expr* eval_list(Env* env, Expr* list) {
    SL_ASSERT(expr_is_proper_list(list));

    Expr dummy_copy;
    dummy_copy.val.pair.cdr = g_nil;
    Expr* cur_copy          = &dummy_copy;

    for (; !expr_is_nil(list); list = CDR(list)) {
        /*
         * Evaluate each argument. If one of them returns an error, propagate it
         * upwards.
         *
         * Otherwise, save the evaluation in our copy, and move to the next
         * argument in our linked list.
         */
        Expr* evaluated = eval(env, CAR(list));
        if (EXPR_ERR_P(evaluated))
            return evaluated;

        CDR(cur_copy) = expr_new(EXPR_PAIR);
        cur_copy      = CDR(cur_copy);
        CAR(cur_copy) = evaluated;
        CDR(cur_copy) = g_nil;
    }

    return dummy_copy.val.pair.cdr;
}

/*
 * Evaluate a list expression as a function call, applying the (evaluated) `car'
 * to the `cdr'. This function is responsible for evaluating the arguments
 * (using 'eval_list') before applying the function, if necessary.
 *
 * TODO: Keep track of a simple call stack (perhaps with a fixed-size
 * array). Store 'Expr*' pointer and human-readable form (if available, before
 * evaluation). Perhaps this should be handled in 'debug.c' via some 'push',
 * 'pop' and 'print' functions.
 */
static Expr* eval_function_call(Env* env, Expr* e) {
    Expr* car = CAR(e);
    Expr* cdr = CDR(e);

    /* Check if the function is a special form symbol, before evaluating it */
    const bool got_special_form = is_special_form(env, car);

    /*
     * Evaluate the expression representing the function. If the evaluation
     * fails, stop. Note that both the evaluated function and the evaluated list
     * of arguments is returned as an allocated clone, so we must free them when
     * we are done.
     */
    Expr* func = eval(env, car);
    if (EXPR_ERR_P(func))
        return func;
    SL_EXPECT(EXPR_APPLICABLE_P(func),
              "Expected function or macro, got '%s'.",
              exprtype2str(func->type));

    /* Is this function in the `*debug-trace*' list? */
    const bool should_print_trace = debug_is_traced_function(env, func);

    /*
     * Normally, we should evaluate each of the arguments before applying the
     * function. However, this step is skipped if:
     *   - There are no arguments.
     *   - The function is a special form.
     *   - The function is a macro.
     * This boolean will be used when evaluating and freeing.
     */
    const bool should_eval_args =
      (!expr_is_nil(cdr) && !got_special_form && !EXPR_MACRO_P(func));

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
        if (EXPR_ERR_P(args))
            return args;
    } else {
        args = cdr;
    }

    if (should_print_trace)
        debug_trace_print_pre(stdout, car, args);

    /* Apply the evaluated function to the evaluated argument list */
    Expr* applied = apply(env, func, args);
    if (applied == NULL)
        applied = err("Unknown error (?)");

    if (should_print_trace)
        debug_trace_print_post(stdout, applied);

    return applied;
}

Expr* eval(Env* env, Expr* e) {
    if (e == NULL)
        return NULL;

    switch (e->type) {
        case EXPR_PAIR: {
            /* Evaluate the list as a procedure/macro call */
            SL_EXPECT(expr_is_proper_list(e),
                      "Expected a proper list for the procedure/macro call.");
            return eval_function_call(env, e);
        }

        case EXPR_SYMBOL: {
            /* Symbols evaluate to the bound value in the current environment */
            Expr* val = env_get(env, e->val.s);
            SL_EXPECT(val != NULL, "Unbound symbol: `%s'.", e->val.s);
            return val;
        }

        case EXPR_ERR:
        case EXPR_NUM_INT:
        case EXPR_NUM_FLT:
        case EXPR_STRING:
        case EXPR_PRIM:
        case EXPR_LAMBDA:
        case EXPR_MACRO:
            /* Not a parent nor a symbol, evaluates to itself */
            return expr_clone(e);

        case EXPR_UNKNOWN:
            SL_FATAL("Tried to evaluate an expression of type 'Unknown'.");
    }

    SL_FATAL("Reached unexpected point, didn't return from switch.");
}

/*----------------------------------------------------------------------------*/

Expr* apply(Env* env, Expr* func, Expr* args) {
    /*
     * Some important notes about the implementation of 'apply':
     *   - It expects a valid environment and a valid applicable function (see
     *     the 'EXPRP_APPLICABLE' function in "expr.h").
     *   - The arguments, are expected to be evaluated by the caller whenever
     *     necessary. The arguments are passed to the function unchanged.
     *   - The 'args' pointer can be NULL, since some functions expect no
     *     arguments. Again, the pointer is passed as-is.
     */
    SL_ASSERT(env != NULL);
    SL_ASSERT(func != NULL);
    SL_ASSERT(EXPR_APPLICABLE_P(func));
    SL_ASSERT(expr_is_proper_list(args));

    Expr* result;
    switch (func->type) {
        case EXPR_PRIM: {
            /* Get primitive C function from the expression */
            PrimitiveFuncPtr primitive = func->val.prim;
            SL_ASSERT(primitive != NULL);

            /*
             * Call primitive C function with the evaluated arguments we got
             * from 'eval'.
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
            result = err("Expected 'Primitive', 'Lambda' or 'Macro'; got '%s'.",
                         exprtype2str(func->type));
        } break;
    }

    return result;
}
