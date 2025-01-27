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
#include <string.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/util.h"
#include "include/memory.h"
#include "include/primitives.h"

/*
 * Used by 'prim_append' when receiving list arguments.
 */
static Expr* list_append(Expr* args) {
    Expr* result = g_nil;

    for (; !expr_is_nil(args); args = CDR(args)) {
        const Expr* arg = CAR(args);
        SL_ASSERT(expr_is_proper_list(arg));
        if (expr_is_nil(arg))
            continue;

        result = expr_nconc(result, expr_clone_tree(arg));
    }

    return result;
}

/* Used by 'prim_append' when receiving string arguments */
static Expr* string_append(Expr* args) {
    /*
     * Calculate the sum of the string lengths, allocate the destination buffer
     * and  concatenate each string using 'stpcpy'.
     *
     * Since 'stpcpy' returns a pointer to the null-terminator, we can store it
     * and keep calling the function repeatedly instead of calculating the
     * string length each iteration, which is probably what 'strcat' does
     * internally.
     */
    size_t total_len = 0;
    for (const Expr* rem = args; !expr_is_nil(rem); rem = CDR(rem)) {
        const Expr* arg = CAR(rem);
        SL_ASSERT(EXPR_STRING_P(arg));
        SL_ASSERT(arg->val.s != NULL);

        total_len += strlen(arg->val.s);
    }

    Expr* ret  = expr_new(EXPR_STRING);
    ret->val.s = mem_alloc(total_len + 1);

    char* last_copied = ret->val.s;
    for (const Expr* rem = args; !expr_is_nil(rem); rem = CDR(rem))
        last_copied = stpcpy(last_copied, CAR(rem)->val.s);

    return ret;
}

/*----------------------------------------------------------------------------*/

/*
 * TODO: This doesn't need to be a primitive, we can just `cons' the arguments
 * with 'nil'. We should note all unnecessary primitives in 'primitives.h'.
 */
Expr* prim_list(Env* env, Expr* args) {
    SL_UNUSED(env);

    /*
     * (list)          ===> nil
     * (list 'a 'b 'c) ===> (a b c)
     */
    return expr_clone_tree(args);
}

/*
 * TODO: Don't create copies, store the reference directly.
 */
Expr* prim_cons(Env* env, Expr* args) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(args, 2);

    /*
     * (cons 'a 'b)     ===> (a . b)
     * (cons 'a '(b c)) ===> (a b c)
     * (cons 'a nil)    ===> (a)
     */
    Expr* ret = expr_new(EXPR_PAIR);
    CAR(ret)  = expr_clone_tree(expr_list_nth(args, 1));
    CDR(ret)  = expr_clone_tree(expr_list_nth(args, 2));

    return ret;
}

/*
 * TODO: Don't create a copy, return the reference directly.
 */
Expr* prim_car(Env* env, Expr* args) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(args, 1);

    const Expr* arg = CAR(args);
    SL_EXPECT(EXPR_PAIR_P(arg) || expr_is_nil(arg),
              "Expected an expression of type '%s' or `nil', got '%s'.",
              exprtype2str(EXPR_PAIR),
              exprtype2str(arg->type));

    /*
     * (car nil)          ===> nil
     * (car '(a . b))     ===> a
     * (car '((a b) y z)) ===> (a b)
     */
    if (expr_is_nil(arg))
        return expr_clone(arg);

    return expr_clone_tree(CAR(arg));
}

/*
 * TODO: Don't create a copy, return the reference directly.
 */
Expr* prim_cdr(Env* env, Expr* args) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(args, 1);

    const Expr* arg = CAR(args);
    SL_EXPECT(EXPR_PAIR_P(arg) || expr_is_nil(arg),
              "Expected an expression of type '%s' or `nil', got '%s'.",
              exprtype2str(EXPR_PAIR),
              exprtype2str(arg->type));

    /*
     * (cdr nil)          ===> nil
     * (cdr '(a . b))     ===> b
     * (cdr '(a b c))     ===> (b c)
     * (cdr '((a b) y z)) ===> (y z)
     */
    if (expr_is_nil(arg))
        return expr_clone(arg);

    return expr_clone_tree(CDR(arg));
}

Expr* prim_length(Env* env, Expr* args) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(args, 1);
    const Expr* arg = CAR(args);

    LispInt result;
    if (expr_is_nil(arg)) {
        result = 0;
    } else if (EXPR_PAIR_P(arg)) {
        SL_EXPECT_PROPER_LIST(arg);
        result = expr_list_len(arg);
    } else if (EXPR_STRING_P(arg)) {
        result = strlen(arg->val.s);
    } else {
        return err("Invalid argument of type '%s'.", exprtype2str(arg->type));
    }

    Expr* ret  = expr_new(EXPR_NUM_INT);
    ret->val.n = result;
    return ret;
}

Expr* prim_append(Env* env, Expr* args) {
    SL_UNUSED(env);

    /* (append) ===> nil */
    if (expr_is_nil(args))
        return expr_clone(g_nil);

    if (!expr_list_has_only_lists(args) &&
        !expr_list_has_only_type(args, EXPR_STRING))
        return err("All arguments must be proper lists or strings.");

    const Expr* first_element = CAR(args);

    /*
     * (append nil)               ===> nil
     * (append '(a b) ... '(y z)) ===> (a b ... y z)
     */
    if (expr_is_proper_list(first_element))
        return list_append(args);

    /*
     * (append "")              ===> ""
     * (append "abc" ... "xyz") ===> "abc...xyz"
     */
    if (EXPR_STRING_P(first_element))
        return string_append(args);

    return err("Invalid argument of type '%s'.",
               exprtype2str(first_element->type));
}
