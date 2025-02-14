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

#include "include/env.h"
#include "include/expr.h"
#include "include/util.h"
#include "include/primitives.h"

Expr* prim_equal(Env* env, Expr* args) {
    SL_UNUSED(env);
    SL_EXPECT_MIN_ARG_NUM(args, 2);

    bool result = true;

    /* (A == B == ...) */
    for (; !expr_is_nil(CDR(args)); args = CDR(args)) {
        if (!expr_equal(CAR(args), CADR(args))) {
            result = false;
            break;
        }
    }

    return (result) ? g_tru : g_nil;
}

Expr* prim_equal_num(Env* env, Expr* args) {
    SL_UNUSED(env);
    SL_EXPECT_MIN_ARG_NUM(args, 2);
    SL_EXPECT(expr_list_has_only_numbers(args),
              "Expected only numeric arguments.");

    bool result = true;

    /* (N1 == N2 == ...) */
    for (; !expr_is_nil(CDR(args)); args = CDR(args)) {
        if (expr_get_generic_num(CAR(args)) !=
            expr_get_generic_num(CADR(args))) {
            result = false;
            break;
        }
    }

    return (result) ? g_tru : g_nil;
}

Expr* prim_lt(Env* env, Expr* args) {
    SL_UNUSED(env);
    SL_EXPECT_MIN_ARG_NUM(args, 2);

    bool result = true;

    /* (A < B < ...) */
    for (; !expr_is_nil(CDR(args)); args = CDR(args)) {
        if (!expr_lt(CAR(args), CADR(args))) {
            result = false;
            break;
        }
    }

    return (result) ? g_tru : g_nil;
}

Expr* prim_gt(Env* env, Expr* args) {
    SL_UNUSED(env);
    SL_EXPECT_MIN_ARG_NUM(args, 2);

    bool result = true;

    /* (A > B > ...) */
    for (; !expr_is_nil(CDR(args)); args = CDR(args)) {
        if (!expr_gt(CAR(args), CADR(args))) {
            result = false;
            break;
        }
    }

    return (result) ? g_tru : g_nil;
}
