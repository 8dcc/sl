
#include <stddef.h>
#include <math.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/util.h"
#include "include/primitives.h"

Expr* prim_add(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");
    SL_EXPECT(expr_list_only_contains_numbers(e),
              "Unexpected non-numerical argument.");

    if (expr_list_contains_type(e, EXPR_NUM_FLT)) {
        double total = 0;
        for (Expr* arg = e; arg != NULL; arg = arg->next)
            total += (arg->type == EXPR_NUM_FLT) ? arg->val.f : arg->val.n;

        Expr* ret  = expr_new(EXPR_NUM_FLT);
        ret->val.f = total;
        return ret;
    } else {
        long long total = 0;
        for (Expr* arg = e; arg != NULL; arg = arg->next)
            total += arg->val.n;

        Expr* ret  = expr_new(EXPR_NUM_INT);
        ret->val.n = total;
        return ret;
    }
}

Expr* prim_sub(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");
    SL_EXPECT(expr_list_only_contains_numbers(e),
              "Unexpected non-numerical argument.");

    /*
     * If there is only one argument, negate. Otherwise subtract in order.
     *   (- 5)       ===> -5
     *   (- 5.0)     ===> -5.0
     *   (- 9 5 1)   ===> 3
     *   (- 9 5.0 1) ===> 3.0
     */
    if (expr_list_contains_type(e, EXPR_NUM_FLT)) {
        double total = (e->type == EXPR_NUM_FLT) ? e->val.f : (double)e->val.n;
        if (e->next == NULL) {
            total = -total;
        } else {
            for (Expr* arg = e->next; arg != NULL; arg = arg->next)
                total -= (arg->type == EXPR_NUM_FLT) ? arg->val.f : arg->val.n;
        }

        Expr* ret  = expr_new(EXPR_NUM_FLT);
        ret->val.f = total;
        return ret;
    } else {
        long long total = e->val.n;
        if (e->next == NULL) {
            total = -total;
        } else {
            for (Expr* arg = e->next; arg != NULL; arg = arg->next)
                total -= arg->val.n;
        }

        Expr* ret  = expr_new(EXPR_NUM_INT);
        ret->val.n = total;
        return ret;
    }
}

Expr* prim_mul(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");
    SL_EXPECT(expr_list_only_contains_numbers(e),
              "Unexpected non-numerical argument.");

    if (expr_list_contains_type(e, EXPR_NUM_FLT)) {
        double total = 1;
        for (Expr* arg = e; arg != NULL; arg = arg->next)
            total *= (arg->type == EXPR_NUM_FLT) ? arg->val.f : arg->val.n;

        Expr* ret  = expr_new(EXPR_NUM_FLT);
        ret->val.f = total;
        return ret;
    } else {
        long long total = 1;
        for (Expr* arg = e; arg != NULL; arg = arg->next)
            total *= arg->val.n;

        Expr* ret  = expr_new(EXPR_NUM_INT);
        ret->val.n = total;
        return ret;
    }
}

Expr* prim_div(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");
    SL_EXPECT(expr_list_only_contains_numbers(e),
              "Unexpected non-numerical argument.");

    /*
     * The `div' primitive always returns a double result. For integer division,
     * use `quotient'.
     */
    double total = (e->type == EXPR_NUM_FLT) ? e->val.f : (double)e->val.n;
    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        switch (arg->type) {
            case EXPR_NUM_INT:
                SL_EXPECT(arg->val.n != 0, "Trying to divide by zero.");
                total /= arg->val.n;
                break;

            case EXPR_NUM_FLT:
                SL_EXPECT(arg->val.f != 0, "Trying to divide by zero.");
                total /= arg->val.f;
                break;

            default:
                SL_FATAL("Unhandled numeric type.");
        }
    }

    Expr* ret  = expr_new(EXPR_NUM_FLT);
    ret->val.f = total;
    return ret;
}

Expr* prim_mod(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");
    SL_EXPECT(expr_list_only_contains_numbers(e),
              "Unexpected non-numerical argument.");

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
    double total = (e->type == EXPR_NUM_FLT) ? e->val.f : (double)e->val.n;
    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        switch (arg->type) {
            case EXPR_NUM_INT:
                SL_EXPECT(arg->val.n != 0, "Trying to divide by zero.");
                total = fmod(total, arg->val.n);
                if (arg->val.n < 0 ? total > 0 : total < 0)
                    total += arg->val.n;
                break;

            case EXPR_NUM_FLT:
                SL_EXPECT(arg->val.f != 0, "Trying to divide by zero.");
                total = fmod(total, arg->val.f);
                if (arg->val.f < 0 ? total > 0 : total < 0)
                    total += arg->val.f;
                break;

            default:
                SL_FATAL("Unhandled numeric type.");
        }
    }

    Expr* ret  = expr_new(EXPR_NUM_FLT);
    ret->val.f = total;
    return ret;
}

Expr* prim_quotient(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");
    SL_EXPECT_TYPE(e, EXPR_NUM_INT);

    /*
     * The `quotient' function is just like `/', but it only operates with
     * integers.
     */
    long long total = e->val.n;
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
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");
    SL_EXPECT_TYPE(e, EXPR_NUM_INT);

    /*
     * The `remainder' function is just like `mod', but it only operates with
     * integers. The following should be equal to the `dividend':
     *
     *   (+ (remainder dividend divisor)
     *      (* (quotient dividend divisor) divisor))
     */
    long long total = e->val.n;
    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        SL_EXPECT_TYPE(arg, EXPR_NUM_INT);
        SL_EXPECT(arg->val.n != 0, "Trying to divide by zero.");
        total %= arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = total;
    return ret;
}

Expr* prim_floor(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT(expr_is_number(e), "Expected numeric argument.");

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
