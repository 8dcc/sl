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
    SL_EXPECT(e != NULL, "Missing arguments.");

    long long total = e->val.n;
    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        SL_EXPECT_TYPE(arg, EXPR_NUM_INT);
        total &= arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = total;
    return ret;
}

Expr* prim_bit_or(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(e != NULL, "Missing arguments.");

    long long total = e->val.n;
    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        SL_EXPECT_TYPE(arg, EXPR_NUM_INT);
        total |= arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = total;
    return ret;
}

Expr* prim_bit_xor(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(e != NULL, "Missing arguments.");

    long long total = e->val.n;
    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
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
    SL_EXPECT_TYPE(e, EXPR_NUM_INT);

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = ~(e->val.n);
    return ret;
}

Expr* prim_shr(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 2);
    SL_EXPECT_TYPE(e, EXPR_NUM_INT);
    SL_EXPECT_TYPE(e->next, EXPR_NUM_INT);

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = (e->val.n >> e->next->val.n);
    return ret;
}

Expr* prim_shl(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 2);
    SL_EXPECT_TYPE(e, EXPR_NUM_INT);
    SL_EXPECT_TYPE(e->next, EXPR_NUM_INT);

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = (e->val.n << e->next->val.n);
    return ret;
}
