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
#include <math.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/util.h"
#include "include/primitives.h"

Expr* prim_add(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(e == NULL || expr_list_has_only_numbers(e),
              "Unexpected non-numeric argument.");

    /*
     * If there are no arguments, return zero.
     *   (+) => 0
     * If arguments don't share the same type, operate on generic numbers.
     *   (+ 9 5.0 1) => 15.0
     * Otherwise, add in order.
     *   (+ 5)           => -5
     *   (+ 9 5 1)       => 15
     *   (+ 9.0 5.0 1.0) => 15.0
     */
    Expr* ret;
    if (e == NULL) {
        ret        = expr_new(EXPR_NUM_INT);
        ret->val.n = 0;
    } else if (!expr_list_is_homogeneous(e)) {
        GenericNum total = expr_get_generic_num(e);
        for (Expr* arg = e->next; arg != NULL; arg = arg->next)
            total += expr_get_generic_num(arg);

        ret = expr_new(EXPR_NUM_GENERIC);
        expr_set_generic_num(ret, total);
    } else if (e->type == EXPR_NUM_INT) {
        LispInt total = e->val.n;
        for (Expr* arg = e->next; arg != NULL; arg = arg->next)
            total += arg->val.n;

        ret        = expr_new(EXPR_NUM_INT);
        ret->val.n = total;
    } else if (e->type == EXPR_NUM_FLT) {
        LispFlt total = e->val.f;
        for (Expr* arg = e->next; arg != NULL; arg = arg->next)
            total += arg->val.f;

        ret        = expr_new(EXPR_NUM_FLT);
        ret->val.f = total;
    } else {
        SL_FATAL("Unhandled numeric type (%s).", exprtype2str(e->type));
    }

    return ret;
}

Expr* prim_sub(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(e == NULL || expr_list_has_only_numbers(e),
              "Unexpected non-numeric argument.");

    /*
     * If there are no arguments, return zero.
     *   (-) => 0
     * If there is only one argument, negate.
     *   (- 5)   => -5
     *   (- 5.0) => -5.0
     * If arguments don't share the same type, operate on generic numbers.
     *   (- 9 5.0 1) => 3.0
     * Otherwise, subtract in order.
     *   (- 9 5 1)       => 3
     *   (- 9.0 5.0 1.0) => 3.0
     */
    Expr* ret;
    if (e == NULL) {
        ret        = expr_new(EXPR_NUM_INT);
        ret->val.n = 0;
    } else if (e->next == NULL) {
        ret = expr_clone(e);
        expr_negate_num_val(ret);
    } else if (!expr_list_is_homogeneous(e)) {
        GenericNum total = expr_get_generic_num(e);
        for (Expr* arg = e->next; arg != NULL; arg = arg->next)
            total -= expr_get_generic_num(arg);

        ret = expr_new(EXPR_NUM_GENERIC);
        expr_set_generic_num(ret, total);
    } else if (e->type == EXPR_NUM_INT) {
        LispInt total = e->val.n;
        for (Expr* arg = e->next; arg != NULL; arg = arg->next)
            total -= arg->val.n;

        ret        = expr_new(EXPR_NUM_INT);
        ret->val.n = total;
    } else if (e->type == EXPR_NUM_FLT) {
        LispFlt total = e->val.f;
        for (Expr* arg = e->next; arg != NULL; arg = arg->next)
            total -= arg->val.f;

        ret        = expr_new(EXPR_NUM_FLT);
        ret->val.f = total;
    } else {
        SL_FATAL("Unhandled numeric type (%s).", exprtype2str(e->type));
    }

    return ret;
}

Expr* prim_mul(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(e == NULL || expr_list_has_only_numbers(e),
              "Unexpected non-numeric argument.");

    /*
     * If there are no arguments, return one.
     *   (*) => 1
     * If there is only one argument, negate.
     *   (* 5)   => -5
     *   (* 5.0) => -5.0
     * If arguments don't share the same type, operate on generic numbers.
     *   (* 9 5.0 1) => 3.0
     * Otherwise, subtract in order.
     *   (* 9 5 1)       => 3
     *   (* 9.0 5.0 1.0) => 3.0
     */
    Expr* ret;
    if (e == NULL) {
        ret        = expr_new(EXPR_NUM_INT);
        ret->val.n = 1;
    } else if (!expr_list_is_homogeneous(e)) {
        GenericNum total = expr_get_generic_num(e);
        for (Expr* arg = e->next; arg != NULL; arg = arg->next)
            total *= expr_get_generic_num(arg);

        ret = expr_new(EXPR_NUM_GENERIC);
        expr_set_generic_num(ret, total);
    } else if (e->type == EXPR_NUM_INT) {
        LispInt total = e->val.n;
        for (Expr* arg = e->next; arg != NULL; arg = arg->next)
            total *= arg->val.n;

        ret        = expr_new(EXPR_NUM_INT);
        ret->val.n = total;
    } else if (e->type == EXPR_NUM_FLT) {
        LispFlt total = e->val.f;
        for (Expr* arg = e->next; arg != NULL; arg = arg->next)
            total *= arg->val.f;

        ret        = expr_new(EXPR_NUM_FLT);
        ret->val.f = total;
    } else {
        SL_FATAL("Unhandled numeric type (%s).", exprtype2str(e->type));
    }

    return ret;
}

Expr* prim_div(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(e != NULL, "Expected at least one argument.");
    SL_EXPECT(expr_list_has_only_numbers(e),
              "Unexpected non-numeric argument.");

    /*
     * The `div' primitive always returns a `GenericNum' result. For integer
     * division, use `quotient'.
     */
    GenericNum total = expr_get_generic_num(e);
    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        const GenericNum n = expr_get_generic_num(arg);
        SL_EXPECT(n != 0, "Trying to divide by zero.");
        total /= n;
    }

    Expr* ret = expr_new(EXPR_NUM_GENERIC);
    expr_set_generic_num(ret, total);
    return ret;
}

Expr* prim_mod(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(e != NULL, "Expected at least one argument.");
    SL_EXPECT(expr_list_has_only_numbers(e),
              "Unexpected non-numeric argument.");

    /*
     * The `mod' operation allows floating-point and negative inputs.
     * See: https://8dcc.github.io/programming/fmod.html
     *
     * Similarly to how the Elisp manual describes `mod', the following should
     * be equal to the `dividend':
     *
     *   (+ (mod dividend divisor)
     *      (* (floor (/ dividend divisor)) divisor))
     *
     * Note that, although the behavior of `mod' in SL is the same as in Elisp,
     * the `floor' and `/' functions are not.
     */
    GenericNum total = expr_get_generic_num(e);
    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        const GenericNum num = expr_get_generic_num(arg);
        SL_EXPECT(num != 0, "Trying to divide by zero.");
        total = fmod(total, num);
        if (num < 0 ? total > 0 : total < 0)
            total += num;
    }

    Expr* ret = expr_new(EXPR_NUM_GENERIC);
    expr_set_generic_num(ret, total);
    return ret;
}

Expr* prim_quotient(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(e != NULL, "Expected at least one argument.");

    /*
     * The `quotient' function is just like `/', but it only operates with
     * integers.
     */
    SL_EXPECT_TYPE(e, EXPR_NUM_INT);
    LispInt total = e->val.n;
    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        SL_EXPECT_TYPE(arg, EXPR_NUM_INT);
        SL_EXPECT(arg->val.n != 0, "Trying to divide by zero.");
        total /= arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = total;
    return ret;
}

Expr* prim_remainder(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(e != NULL, "Missing arguments.");

    /*
     * The `remainder' function is just like `mod', but it only operates with
     * integers. The following should be equal to the `dividend':
     *
     *   (+ (remainder dividend divisor)
     *      (* (quotient dividend divisor) divisor))
     */
    SL_EXPECT_TYPE(e, EXPR_NUM_INT);
    LispInt total = e->val.n;
    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        SL_EXPECT_TYPE(arg, EXPR_NUM_INT);
        SL_EXPECT(arg->val.n != 0, "Trying to divide by zero.");
        total %= arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = total;
    return ret;
}

Expr* prim_round(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT(EXPRP_NUMBER(e), "Expected numeric argument.");

    Expr* ret = expr_new(e->type);
    switch (e->type) {
        case EXPR_NUM_INT:
            ret->val.n = e->val.n;
            break;
        case EXPR_NUM_FLT:
            ret->val.f = round(e->val.f);
            break;
        default:
            SL_FATAL("Unhandled numeric type.");
    }
    return ret;
}

Expr* prim_floor(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT(EXPRP_NUMBER(e), "Expected numeric argument.");

    Expr* ret = expr_new(e->type);
    switch (e->type) {
        case EXPR_NUM_INT:
            ret->val.n = e->val.n;
            break;
        case EXPR_NUM_FLT:
            ret->val.f = floor(e->val.f);
            break;
        default:
            SL_FATAL("Unhandled numeric type.");
    }
    return ret;
}

Expr* prim_ceiling(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT(EXPRP_NUMBER(e), "Expected numeric argument.");

    Expr* ret = expr_new(e->type);
    switch (e->type) {
        case EXPR_NUM_INT:
            ret->val.n = e->val.n;
            break;
        case EXPR_NUM_FLT:
            ret->val.f = ceil(e->val.f);
            break;
        default:
            SL_FATAL("Unhandled numeric type.");
    }
    return ret;
}

Expr* prim_truncate(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT(EXPRP_NUMBER(e), "Expected numeric argument.");

    Expr* ret = expr_new(e->type);
    switch (e->type) {
        case EXPR_NUM_INT:
            ret->val.n = e->val.n;
            break;
        case EXPR_NUM_FLT:
            ret->val.f = trunc(e->val.f);
            break;
        default:
            SL_FATAL("Unhandled numeric type.");
    }
    return ret;
}
