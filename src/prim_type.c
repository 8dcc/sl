
#include <stddef.h>
#include <stdbool.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/util.h"
#include "include/primitives.h"

/*----------------------------------------------------------------------------*/
/* Type-conversion primitives */

Expr* prim_int2flt(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_NUM_INT);

    Expr* ret  = expr_new(EXPR_NUM_FLT);
    ret->val.f = (double)e->val.n;
    return ret;
}

Expr* prim_flt2int(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_NUM_FLT);

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = (long long)e->val.f;
    return ret;
}

Expr* prim_int2str(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_NUM_INT);

    char* s;
    if (int2str(e->val.n, &s) == 0)
        return NULL;

    Expr* ret  = expr_new(EXPR_STRING);
    ret->val.s = s;
    return ret;
}

Expr* prim_flt2str(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_NUM_FLT);

    char* s;
    if (flt2str(e->val.f, &s) == 0)
        return NULL;

    Expr* ret  = expr_new(EXPR_STRING);
    ret->val.s = s;
    return ret;
}

Expr* prim_str2int(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_STRING);

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = strtoll(e->val.s, NULL, STRTOLL_ANY_BASE);
    return ret;
}

Expr* prim_str2flt(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_STRING);

    Expr* ret  = expr_new(EXPR_NUM_FLT);
    ret->val.f = strtod(e->val.s, NULL);
    return ret;
}

/*----------------------------------------------------------------------------*/
/* Type-checking primitives */

Expr* prim_is_int(Env* env, Expr* e) {
    const bool result = expr_list_only_contains_type(e, EXPR_NUM_INT);
    return (result) ? env_get(env, "tru") : env_get(env, "nil");
}

Expr* prim_is_flt(Env* env, Expr* e) {
    const bool result = expr_list_only_contains_type(e, EXPR_NUM_FLT);
    return (result) ? env_get(env, "tru") : env_get(env, "nil");
}

Expr* prim_is_symbol(Env* env, Expr* e) {
    const bool result = expr_list_only_contains_type(e, EXPR_SYMBOL);
    return (result) ? env_get(env, "tru") : env_get(env, "nil");
}

Expr* prim_is_string(Env* env, Expr* e) {
    const bool result = expr_list_only_contains_type(e, EXPR_STRING);
    return (result) ? env_get(env, "tru") : env_get(env, "nil");
}

Expr* prim_is_list(Env* env, Expr* e) {
    const bool result = expr_list_only_contains_type(e, EXPR_PARENT);
    return (result) ? env_get(env, "tru") : env_get(env, "nil");
}

Expr* prim_is_primitive(Env* env, Expr* e) {
    const bool result = expr_list_only_contains_type(e, EXPR_PRIM);
    return (result) ? env_get(env, "tru") : env_get(env, "nil");
}

Expr* prim_is_lambda(Env* env, Expr* e) {
    const bool result = expr_list_only_contains_type(e, EXPR_LAMBDA);
    return (result) ? env_get(env, "tru") : env_get(env, "nil");
}

Expr* prim_is_macro(Env* env, Expr* e) {
    const bool result = expr_list_only_contains_type(e, EXPR_MACRO);
    return (result) ? env_get(env, "tru") : env_get(env, "nil");
}
