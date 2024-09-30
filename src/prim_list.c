
#include <stddef.h>
#include <string.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/util.h"
#include "include/primitives.h"

/* Used by `prim_append' when receiving list arguments */
static Expr* list_append(Expr* e) {
    SL_ASSERT(e != NULL);

    Expr dummy_copy;
    dummy_copy.next = NULL;
    Expr* cur_copy  = &dummy_copy;

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        SL_ASSERT(arg->type == EXPR_PARENT);

        if (arg->val.children == NULL)
            continue;

        cur_copy->next = expr_list_clone(arg->val.children);
        while (cur_copy->next != NULL)
            cur_copy = cur_copy->next;
    }

    Expr* ret         = expr_new(EXPR_PARENT);
    ret->val.children = dummy_copy.next;
    return ret;
}

/* Used by `prim_append' when receiving string arguments */
static Expr* string_append(Expr* e) {
    SL_ASSERT(e != NULL);

    /*
     * Calculate the sum of the string lengths, allocate the destination buffer
     * and  concatenate each string using `stpcpy'.
     *
     * Since `stpcpy' returns a pointer to the null-terminator, we can store it
     * and keep calling the function repeatedly instead of calculating the
     * string length each iteration, which is probably what `strcat' does
     * internally.
     */
    size_t total_len = 0;
    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        SL_ASSERT(arg->type == EXPR_STRING);
        SL_ASSERT(arg->val.s != NULL);

        total_len += strlen(arg->val.s);
    }

    Expr* ret  = expr_new(EXPR_STRING);
    ret->val.s = sl_safe_malloc(total_len + 1);

    char* last_copied = ret->val.s;
    for (Expr* arg = e; arg != NULL; arg = arg->next)
        last_copied = stpcpy(last_copied, arg->val.s);

    return ret;
}

/*----------------------------------------------------------------------------*/

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

Expr* prim_length(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 1);

    long long result;
    switch (e->type) {
        case EXPR_PARENT:
            result = expr_list_len(e->val.children);
            break;

        case EXPR_STRING:
            result = strlen(e->val.s);
            break;

        default:
            SL_WRN("Invalid argument of type '%s'.", exprtype2str(e->type));
            return NULL;
    }

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = result;
    return ret;
}

Expr* prim_append(Env* env, Expr* e) {
    SL_UNUSED(env);

    if (e == NULL) {
        /* (append) ===> nil */
        Expr* ret         = expr_new(EXPR_PARENT);
        ret->val.children = NULL;
        return ret;
    }

    if (!expr_list_is_homogeneous(e)) {
        SL_WRN("Expected arguments of the same type.");
        return NULL;
    }

    Expr* ret;
    switch (e->type) {
        case EXPR_PARENT:
            /*
             * (append nil)               ===> nil
             * (append '(a b) ... '(y z)) ===> (a b ... y z)
             */
            ret = list_append(e);
            break;

        case EXPR_STRING:
            /*
             * (append "")              ===> ""
             * (append "abc" ... "xyz") ===> "abc...xyz"
             */
            ret = string_append(e);
            break;

        default:
            SL_WRN("Invalid argument of type '%s'.", exprtype2str(e->type));
            ret = NULL;
            break;
    }

    return ret;
}
