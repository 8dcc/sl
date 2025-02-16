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

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/lambda.h"
#include "include/util.h"
#include "include/eval.h"
#include "include/primitives.h"

Expr* prim_eval(Env* env, Expr* args) {
    SL_EXPECT_ARG_NUM(args, 1);
    return eval(env, CAR(args));
}

Expr* prim_apply(Env* env, Expr* args) {
    SL_EXPECT_ARG_NUM(args, 2);

    Expr* func      = expr_list_nth(args, 1);
    Expr* func_args = expr_list_nth(args, 2);
    SL_EXPECT(EXPR_APPLICABLE_P(func),
              "Expected a function or macro as the first argument, got '%s'.",
              exprtype2str(func->type));
    SL_EXPECT(expr_is_proper_list(func_args),
              "Expected a list of arguments, got '%s'.",
              exprtype2str(func_args->type));

    return apply(env, func, func_args);
}

Expr* prim_macroexpand(Env* env, Expr* args) {
    SL_EXPECT_ARG_NUM(args, 1);

    Expr* call_expr = CAR(args);
    SL_EXPECT_PROPER_LIST(call_expr);
    SL_EXPECT(expr_list_len(call_expr) >= 1,
              "The supplied list must have at least one element: The macro "
              "representation.");

    Expr* macro_representation = CAR(call_expr);
    Expr* macro_args           = CDR(call_expr);

    /*
     * Similar to how function calls are evaluated in the 'eval_function_call'
     * static function in "eval.c", but the arguments are never evaluated.
     *
     * Note that we expect a quoted expression, so neither the arguments nor the
     * macro should have been evaluated.
     */
    Expr* macro = eval(env, macro_representation);
    if (EXPR_ERR_P(macro))
        return macro;
    SL_EXPECT_TYPE(macro, EXPR_MACRO);

    return macro_expand(env, macro, macro_args);
}

Expr* prim_set(Env* env, Expr* args) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(args, 2);
    Expr* dst       = CAR(args);
    const Expr* src = CADR(args);

    /*
     * The `set' primitive copies the value of the "source" argument into the
     * "destination". Note that it doesn't replace the reference of the
     * destination, it replaces its entire value and type.
     */
    expr_set(dst, src);
    return dst;
}

Expr* prim_random(Env* env, Expr* args) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(args, 1);

    const Expr* limit = CAR(args);
    SL_EXPECT(EXPR_NUMBER_P(limit), "Expected numeric argument.");

    /*
     * We return the same numeric type we received.
     *
     * For more information on the value ranges, see:
     * https://c-faq.com/lib/randrange.html
     */
    Expr* ret = expr_new(limit->type);
    switch (limit->type) {
        case EXPR_NUM_INT:
            ret->val.n = rand() / (RAND_MAX / limit->val.n + 1);
            break;

        case EXPR_NUM_FLT:
            ret->val.f = (double)rand() / ((double)RAND_MAX + 1) * limit->val.f;
            break;

        default:
            SL_FATAL("Unhandled numeric type.");
    }

    return ret;
}

Expr* prim_set_random_seed(Env* env, Expr* args) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(args, 1);

    const Expr* seed = CAR(args);
    SL_EXPECT_TYPE(seed, EXPR_NUM_INT);

    srand(seed->val.n);
    return g_tru;
}
