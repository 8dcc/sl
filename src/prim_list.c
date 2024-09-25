
#include <stddef.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/util.h"
#include "include/primitives.h"

Expr* prim_list(Env* env, Expr* e) {
    SL_UNUSED(env);

    /*
     * (list)          ===> nil
     * (list 'a 'b 'c) ===> (a b c)
     */
    Expr* ret         = expr_new(EXPR_PARENT);
    ret->val.children = expr_list_clone(e);

    return ret;
}

Expr* prim_cons(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 2);
    SL_EXPECT_TYPE(e->next, EXPR_PARENT);

    /*
     * (cons a nil)    ===> (a)
     * (cons a '(b c)) ===> (a b c)
     */
    Expr* ret         = expr_new(EXPR_PARENT);
    ret->val.children = expr_clone_recur(e);

    if (e->next->val.children != NULL)
        ret->val.children->next = expr_list_clone(e->next->val.children);

    return ret;
}

Expr* prim_car(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_PARENT);

    /*
     * (car '())          ===> nil
     * (car '(a b c))     ===> a
     * (car '((a b) y z)) ===> (a b)
     */
    if (expr_is_nil(e))
        return expr_clone(e);

    return expr_clone_recur(e->val.children);
}

Expr* prim_cdr(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_PARENT);

    /*
     * (cdr '())          ===> nil
     * (cdr '(a))         ===> nil
     * (cdr '(a b c))     ===> (b c)
     * (cdr '((a b) y z)) ===> (y z)
     */
    Expr* ret = expr_new(EXPR_PARENT);

    if (e->val.children == NULL || e->val.children->next == NULL)
        ret->val.children = NULL;
    else
        ret->val.children = expr_list_clone(e->val.children->next);

    return ret;
}

Expr* prim_append(Env* env, Expr* e) {
    SL_UNUSED(env);

    /*
     * (append)                   ===> nil
     * (append nil)               ===> nil
     * (append '(a b) ... '(y z)) ===> (a b ... y z)
     */
    Expr* ret = expr_new(EXPR_PARENT);
    if (e == NULL) {
        ret->val.children = NULL;
        return ret;
    }

    SL_ON_ERR(return ret);

    Expr dummy_copy;
    dummy_copy.next = NULL;
    Expr* cur_copy  = &dummy_copy;

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        SL_EXPECT_TYPE(arg, EXPR_PARENT);

        if (arg->val.children == NULL)
            continue;

        cur_copy->next = expr_list_clone(arg->val.children);
        while (cur_copy->next != NULL)
            cur_copy = cur_copy->next;
    }

    ret->val.children = dummy_copy.next;
    return ret;
}
