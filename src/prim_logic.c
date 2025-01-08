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

Expr* prim_equal(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(expr_list_len(e) >= 2, "Expected at least 2 arguments.");

    bool result = true;

    /* (A == B == ...) */
    for (Expr* arg = e; arg->next != NULL; arg = arg->next) {
        if (!expr_equal(arg, arg->next)) {
            result = false;
            break;
        }
    }

    return (result) ? expr_clone(g_tru) : expr_clone(g_nil);
}

Expr* prim_equal_num(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(expr_list_len(e) >= 2, "Expected at least 2 arguments.");

    bool result = true;

    /* (N1 == N2 == ...) */
    for (Expr* arg = e; arg->next != NULL; arg = arg->next) {
        SL_EXPECT(EXPRP_NUMBER(arg), "Unexpected non-numeric argument.");
        if (expr_get_generic_num(arg) != expr_get_generic_num(arg->next)) {
            result = false;
            break;
        }
    }

    return (result) ? expr_clone(g_tru) : expr_clone(g_nil);
}

Expr* prim_lt(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(expr_list_len(e) >= 2, "Expected at least 2 arguments.");

    bool result = true;

    /* (A < B < ...) */
    for (Expr* arg = e; arg->next != NULL; arg = arg->next) {
        if (!expr_lt(arg, arg->next)) {
            result = false;
            break;
        }
    }

    return (result) ? expr_clone(g_tru) : expr_clone(g_nil);
}

Expr* prim_gt(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(expr_list_len(e) >= 2, "Expected at least 2 arguments.");

    bool result = true;

    /* (A > B > ...) */
    for (Expr* arg = e; arg->next != NULL; arg = arg->next) {
        if (!expr_gt(arg, arg->next)) {
            result = false;
            break;
        }
    }

    return (result) ? expr_clone(g_tru) : expr_clone(g_nil);
}
