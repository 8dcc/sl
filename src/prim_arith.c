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

    const bool no_args = expr_is_nil(e);
    SL_EXPECT(no_args || expr_list_has_only_numbers(e),
              "Unexpected non-numeric argument.");

    const Expr* first_arg = CAR(e);

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
    if (no_args) {
        ret        = expr_new(EXPR_NUM_INT);
        ret->val.n = 0;
    } else if (!expr_list_is_homogeneous(e)) {
        GenericNum total = 0;
        for (; !expr_is_nil(e); e = CDR(e))
            total += expr_get_generic_num(CAR(e));

        ret = expr_new(EXPR_NUM_GENERIC);
        expr_set_generic_num(ret, total);
    } else if (first_arg->type == EXPR_NUM_INT) {
        LispInt total = 0;
        for (; !expr_is_nil(e); e = CDR(e))
            total += CAR(e)->val.n;

        ret        = expr_new(EXPR_NUM_INT);
        ret->val.n = total;
    } else if (first_arg->type == EXPR_NUM_FLT) {
        LispFlt total = 0.0;
        for (; !expr_is_nil(e); e = CDR(e))
            total += CAR(e)->val.f;

        ret        = expr_new(EXPR_NUM_FLT);
        ret->val.f = total;
    } else {
        SL_FATAL("Unhandled numeric type (%s).", exprtype2str(first_arg->type));
    }

    return ret;
}

Expr* prim_sub(Env* env, Expr* e) {
    SL_UNUSED(env);

    const bool no_args = expr_is_nil(e);
    SL_EXPECT(no_args || expr_list_has_only_numbers(e),
              "Unexpected non-numeric argument.");

    const Expr* first_arg = CAR(e);

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
    if (no_args) {
        ret        = expr_new(EXPR_NUM_INT);
        ret->val.n = 0;
    } else if (expr_is_nil(CDR(e))) {
        ret = expr_clone(first_arg);
        expr_negate_num_val(ret);
    } else if (!expr_list_is_homogeneous(e)) {
        GenericNum total = expr_get_generic_num(first_arg);
        for (e = CDR(e); !expr_is_nil(e); e = CDR(e))
            total -= expr_get_generic_num(CAR(e));

        ret = expr_new(EXPR_NUM_GENERIC);
        expr_set_generic_num(ret, total);
    } else if (first_arg->type == EXPR_NUM_INT) {
        LispInt total = first_arg->val.n;
        for (e = CDR(e); !expr_is_nil(e); e = CDR(e))
            total -= CAR(e)->val.n;

        ret        = expr_new(EXPR_NUM_INT);
        ret->val.n = total;
    } else if (first_arg->type == EXPR_NUM_FLT) {
        LispFlt total = first_arg->val.f;
        for (e = CDR(e); !expr_is_nil(e); e = CDR(e))
            total -= CAR(e)->val.f;

        ret        = expr_new(EXPR_NUM_FLT);
        ret->val.f = total;
    } else {
        SL_FATAL("Unhandled numeric type (%s).", exprtype2str(first_arg->type));
    }

    return ret;
}

Expr* prim_mul(Env* env, Expr* e) {
    SL_UNUSED(env);

    const bool no_args = expr_is_nil(e);
    SL_EXPECT(no_args || expr_list_has_only_numbers(e),
              "Unexpected non-numeric argument.");

    const Expr* first_arg = CAR(e);

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
    if (no_args) {
        ret        = expr_new(EXPR_NUM_INT);
        ret->val.n = 1;
    } else if (!expr_list_is_homogeneous(e)) {
        GenericNum total = expr_get_generic_num(first_arg);
        for (e = CDR(e); !expr_is_nil(e); e = CDR(e))
            total *= expr_get_generic_num(CAR(e));

        ret = expr_new(EXPR_NUM_GENERIC);
        expr_set_generic_num(ret, total);
    } else if (first_arg->type == EXPR_NUM_INT) {
        LispInt total = first_arg->val.n;
        for (e = CDR(e); !expr_is_nil(e); e = CDR(e))
            total *= CAR(e)->val.n;

        ret        = expr_new(EXPR_NUM_INT);
        ret->val.n = total;
    } else if (first_arg->type == EXPR_NUM_FLT) {
        LispFlt total = first_arg->val.f;
        for (e = CDR(e); !expr_is_nil(e); e = CDR(e))
            total *= CAR(e)->val.f;

        ret        = expr_new(EXPR_NUM_FLT);
        ret->val.f = total;
    } else {
        SL_FATAL("Unhandled numeric type (%s).", exprtype2str(first_arg->type));
    }

    return ret;
}

Expr* prim_div(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(!expr_is_nil(e), "Expected at least one argument.");
    SL_EXPECT(expr_list_has_only_numbers(e),
              "Unexpected non-numeric argument.");

    /*
     * The `div' primitive always returns a 'GenericNum' result. For integer
     * division, use `quotient'.
     */
    GenericNum total = expr_get_generic_num(CAR(e));
    for (e = CDR(e); !expr_is_nil(e); e = CDR(e)) {
        const GenericNum n = expr_get_generic_num(CAR(e));
        SL_EXPECT(n != 0, "Trying to divide by zero.");
        total /= n;
    }

    Expr* ret = expr_new(EXPR_NUM_GENERIC);
    expr_set_generic_num(ret, total);
    return ret;
}

Expr* prim_mod(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT(!expr_is_nil(e), "Expected at least one argument.");
    SL_EXPECT(expr_list_has_only_numbers(e),
              "Unexpected non-numeric argument.");

    /*
     * The `mod' operation allows floating-point and negative inputs.
     * See: https://8dcc.github.io/programming/fmod.html
     *
     * Similarly to how the Elisp manual describes `mod', the following should
     * be equal to the 'dividend':
     *
     *   (+ (mod dividend divisor)
     *      (* (floor (/ dividend divisor)) divisor))
     *
     * Note that, although the behavior of `mod' in SL is the same as in Elisp,
     * the `floor' and `/' functions are not.
     */
    GenericNum total = expr_get_generic_num(CAR(e));
    for (e = CDR(e); !expr_is_nil(e); e = CDR(e)) {
        const GenericNum num = expr_get_generic_num(CAR(e));
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
    SL_EXPECT(!expr_is_nil(e), "Expected at least one argument.");

    const Expr* first_arg = CAR(e);
    SL_EXPECT_TYPE(first_arg, EXPR_NUM_INT);

    /*
     * The `quotient' function is just like `/', but it only operates with
     * integers.
     */
    LispInt total = first_arg->val.n;
    for (e = CDR(e); !expr_is_nil(e); e = CDR(e)) {
        const Expr* arg = CAR(e);
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
    SL_EXPECT(!expr_is_nil(e), "Expected at least one argument.");

    const Expr* first_arg = CAR(e);
    SL_EXPECT_TYPE(first_arg, EXPR_NUM_INT);

    /*
     * The `remainder' function is just like `mod', but it only operates with
     * integers. The following should be equal to the `dividend':
     *
     *   (+ (remainder dividend divisor)
     *      (* (quotient dividend divisor) divisor))
     */
    LispInt total = first_arg->val.n;
    for (e = CDR(e); !expr_is_nil(e); e = CDR(e)) {
        const Expr* arg = CAR(e);
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

    const Expr* arg = CAR(e);
    SL_EXPECT(EXPR_NUMBER_P(arg), "Expected numeric argument.");

    Expr* ret = expr_new(arg->type);
    switch (arg->type) {
        case EXPR_NUM_INT:
            ret->val.n = arg->val.n;
            break;
        case EXPR_NUM_FLT:
            ret->val.f = round(arg->val.f);
            break;
        default:
            SL_FATAL("Unhandled numeric type.");
    }
    return ret;
}

Expr* prim_floor(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 1);

    const Expr* arg = CAR(e);
    SL_EXPECT(EXPR_NUMBER_P(arg), "Expected numeric argument.");

    Expr* ret = expr_new(arg->type);
    switch (arg->type) {
        case EXPR_NUM_INT:
            ret->val.n = arg->val.n;
            break;
        case EXPR_NUM_FLT:
            ret->val.f = floor(arg->val.f);
            break;
        default:
            SL_FATAL("Unhandled numeric type.");
    }
    return ret;
}

Expr* prim_ceiling(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 1);

    const Expr* arg = CAR(e);
    SL_EXPECT(EXPR_NUMBER_P(arg), "Expected numeric argument.");

    Expr* ret = expr_new(arg->type);
    switch (arg->type) {
        case EXPR_NUM_INT:
            ret->val.n = arg->val.n;
            break;
        case EXPR_NUM_FLT:
            ret->val.f = ceil(arg->val.f);
            break;
        default:
            SL_FATAL("Unhandled numeric type.");
    }
    return ret;
}

Expr* prim_truncate(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(e, 1);

    const Expr* arg = CAR(e);
    SL_EXPECT(EXPR_NUMBER_P(arg), "Expected numeric argument.");

    Expr* ret = expr_new(arg->type);
    switch (arg->type) {
        case EXPR_NUM_INT:
            ret->val.n = arg->val.n;
            break;
        case EXPR_NUM_FLT:
            ret->val.f = trunc(arg->val.f);
            break;
        default:
            SL_FATAL("Unhandled numeric type.");
    }
    return ret;
}
