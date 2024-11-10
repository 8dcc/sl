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
#include <stdbool.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/util.h"
#include "include/memory.h"
#include "include/primitives.h"

/*----------------------------------------------------------------------------*/
/* Type-checking primitives */

Expr* prim_type_of(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 1);

    Expr* ret  = expr_new(EXPR_SYMBOL);
    ret->val.s = mem_strdup(exprtype2str(e->type));
    return ret;
}

Expr* prim_is_int(Env* env, Expr* e) {
    SL_UNUSED(env);
    const bool result = expr_list_has_only_type(e, EXPR_NUM_INT);
    return (result) ? expr_clone(tru) : expr_clone(nil);
}

Expr* prim_is_flt(Env* env, Expr* e) {
    SL_UNUSED(env);
    const bool result = expr_list_has_only_type(e, EXPR_NUM_FLT);
    return (result) ? expr_clone(tru) : expr_clone(nil);
}

Expr* prim_is_symbol(Env* env, Expr* e) {
    SL_UNUSED(env);
    const bool result = expr_list_has_only_type(e, EXPR_SYMBOL);
    return (result) ? expr_clone(tru) : expr_clone(nil);
}

Expr* prim_is_string(Env* env, Expr* e) {
    SL_UNUSED(env);
    const bool result = expr_list_has_only_type(e, EXPR_STRING);
    return (result) ? expr_clone(tru) : expr_clone(nil);
}

Expr* prim_is_list(Env* env, Expr* e) {
    SL_UNUSED(env);
    const bool result = expr_list_has_only_type(e, EXPR_PARENT);
    return (result) ? expr_clone(tru) : expr_clone(nil);
}

Expr* prim_is_primitive(Env* env, Expr* e) {
    SL_UNUSED(env);
    const bool result = expr_list_has_only_type(e, EXPR_PRIM);
    return (result) ? expr_clone(tru) : expr_clone(nil);
}

Expr* prim_is_lambda(Env* env, Expr* e) {
    SL_UNUSED(env);
    const bool result = expr_list_has_only_type(e, EXPR_LAMBDA);
    return (result) ? expr_clone(tru) : expr_clone(nil);
}

Expr* prim_is_macro(Env* env, Expr* e) {
    SL_UNUSED(env);
    const bool result = expr_list_has_only_type(e, EXPR_MACRO);
    return (result) ? expr_clone(tru) : expr_clone(nil);
}

/*----------------------------------------------------------------------------*/
/* Type conversion primitives */

Expr* prim_int2flt(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_NUM_INT);

    Expr* ret  = expr_new(EXPR_NUM_FLT);
    ret->val.f = (double)e->val.n;
    return ret;
}

Expr* prim_flt2int(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_NUM_FLT);

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = (long long)e->val.f;
    return ret;
}

/*
 * TODO: Are these `x2str' primitives necessary? Why not simply use
 * `write-to-str'?
 */
Expr* prim_int2str(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_NUM_INT);

    char* s;
    const size_t written = int2str(e->val.n, &s);
    SL_EXPECT(written > 0, "Failed to convert Integer to String.");

    Expr* ret  = expr_new(EXPR_STRING);
    ret->val.s = s;
    return ret;
}

Expr* prim_flt2str(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_NUM_FLT);

    char* s;
    const size_t written = flt2str(e->val.f, &s);
    SL_EXPECT(written > 0, "Failed to convert Float to String.");

    Expr* ret  = expr_new(EXPR_STRING);
    ret->val.s = s;
    return ret;
}

Expr* prim_str2int(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_STRING);

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = strtoll(e->val.s, NULL, STRTOLL_ANY_BASE);
    return ret;
}

Expr* prim_str2flt(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_STRING);

    Expr* ret  = expr_new(EXPR_NUM_FLT);
    ret->val.f = strtod(e->val.s, NULL);
    return ret;
}
