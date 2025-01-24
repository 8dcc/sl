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

Expr* prim_bit_and(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(!expr_is_nil(e), "Expected at least one argument.");

    LispInt total = CAR(e)->val.n;
    for (e = CDR(e); !expr_is_nil(e); e = CDR(e)) {
        const Expr* arg = CAR(e);
        SL_EXPECT_TYPE(arg, EXPR_NUM_INT);
        total &= arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = total;
    return ret;
}

Expr* prim_bit_or(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(!expr_is_nil(e), "Expected at least one argument.");

    LispInt total = CAR(e)->val.n;
    for (e = CDR(e); !expr_is_nil(e); e = CDR(e)) {
        const Expr* arg = CAR(e);
        SL_EXPECT_TYPE(arg, EXPR_NUM_INT);
        total |= arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = total;
    return ret;
}

Expr* prim_bit_xor(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(!expr_is_nil(e), "Expected at least one argument.");

    LispInt total = CAR(e)->val.n;
    for (e = CDR(e); !expr_is_nil(e); e = CDR(e)) {
        const Expr* arg = CAR(e);
        SL_EXPECT_TYPE(arg, EXPR_NUM_INT);
        total ^= arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = total;
    return ret;
}

Expr* prim_bit_not(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 1);

    const Expr* arg = CAR(e);
    SL_EXPECT_TYPE(arg, EXPR_NUM_INT);

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = ~(arg->val.n);
    return ret;
}

Expr* prim_shr(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 2);

    const Expr* num = CAR(e);
    SL_EXPECT_TYPE(num, EXPR_NUM_INT);
    const Expr* count = CADR(e);
    SL_EXPECT_TYPE(count, EXPR_NUM_INT);

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = (num->val.n >> count->val.n);
    return ret;
}

Expr* prim_shl(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 2);

    const Expr* num = CAR(e);
    SL_EXPECT_TYPE(num, EXPR_NUM_INT);
    const Expr* count = CADR(e);
    SL_EXPECT_TYPE(count, EXPR_NUM_INT);

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = (num->val.n << count->val.n);
    return ret;
}
