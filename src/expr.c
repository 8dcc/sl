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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/expr_pool.h"
#include "include/lambda.h"
#include "include/util.h"
#include "include/memory.h"

Expr* expr_new(enum EExprType type) {
    Expr* ret = pool_alloc_or_expand(BASE_POOL_SZ);
    ret->type = type;
    memset(&ret->val, 0, sizeof(ret->val));
    return ret;
}

/*----------------------------------------------------------------------------*/

Expr* expr_clone(const Expr* e) {
    if (e == NULL)
        return NULL;

    /*
     * Nothing allocated (e.g. symbol strings) is re-used from the old
     * expression, everything is allocated and copied again.
     *
     * Note that, in the case of pairs, this function doesn't clone the tree
     * recursively, it just copies the old references; see also
     * 'expr_clone_recur'.
     */
    Expr* ret = expr_new(e->type);

    switch (e->type) {
        case EXPR_NUM_INT:
            ret->val.n = e->val.n;
            break;

        case EXPR_NUM_FLT:
            ret->val.f = e->val.f;
            break;

        case EXPR_ERR:
        case EXPR_SYMBOL:
        case EXPR_STRING:
            ret->val.s = mem_strdup(e->val.s);
            break;

        case EXPR_PAIR:
            CAR(ret) = CAR(e);
            CDR(ret) = CDR(e);
            break;

        case EXPR_PRIM:
            ret->val.prim = e->val.prim;
            break;

        case EXPR_MACRO:
        case EXPR_LAMBDA:
            ret->val.lambda = lambdactx_clone(e->val.lambda);
            break;

        case EXPR_UNKNOWN:
            SL_ERR("Trying to clone <unknown>");
            break;
    }

    return ret;
}

Expr* expr_clone_recur(const Expr* e) {
    if (e == NULL)
        return NULL;

    Expr* cloned = expr_clone(e);
    if (cloned->type == EXPR_PAIR) {
        CAR(cloned) = expr_clone_recur(CAR(cloned));
        CDR(cloned) = expr_clone_recur(CDR(cloned));
    }

    return cloned;
}

/*----------------------------------------------------------------------------*/

bool expr_is_nil(const Expr* e) {
    return e != NULL && e->type == EXPR_SYMBOL && e->val.s != NULL &&
           strcmp(e->val.s, "nil") == 0;
}

bool expr_equal(const Expr* a, const Expr* b) {
    /*
     * If one of them is NULL, they are equal if the other is also NULL. This is
     * an important check, specially since when calling ourselves recursively.
     */
    if (a == NULL || b == NULL)
        return a == b;

    /*
     * This function checks for isomorphism, so if they don't share the same
     * type, they are not equal. For comparing numbers, see 'prim_equal_num' or
     * 'expr_lt'.
     */
    if (a->type != b->type)
        return false;

    switch (a->type) {
        case EXPR_NUM_INT:
            return a->val.n == b->val.n;

        case EXPR_NUM_FLT:
            return a->val.f == b->val.f;

        case EXPR_ERR:
        case EXPR_SYMBOL:
        case EXPR_STRING:
            return strcmp(a->val.s, b->val.s) == 0;

        case EXPR_PAIR:
            return expr_equal(CAR(a), CAR(b)) && expr_equal(CDR(a), CDR(b));

        case EXPR_PRIM:
            return a->val.prim == b->val.prim;

        case EXPR_MACRO:
        case EXPR_LAMBDA:
            return lambdactx_equal(a->val.lambda, b->val.lambda);

        case EXPR_UNKNOWN:
            return false;
    }

    __builtin_unreachable();
}

bool expr_lt(const Expr* a, const Expr* b) {
    if (a == NULL || b == NULL)
        return false;

    /* See 'prim_equal_num' */
    if (a->type != b->type)
        return (EXPR_NUMBER_P(a) && EXPR_NUMBER_P(b))
                 ? (expr_get_generic_num(a) < expr_get_generic_num(b))
                 : false;

    if (a->type != b->type)
        return false;

    switch (a->type) {
        case EXPR_NUM_INT:
            return a->val.n < b->val.n;

        case EXPR_NUM_FLT:
            return a->val.f < b->val.f;

        case EXPR_ERR:
        case EXPR_SYMBOL:
        case EXPR_STRING:
            return strcmp(a->val.s, b->val.s) < 0;

        case EXPR_PAIR:
        case EXPR_PRIM:
        case EXPR_LAMBDA:
        case EXPR_MACRO:
        case EXPR_UNKNOWN:
            return false;
    }

    __builtin_unreachable();
}

bool expr_gt(const Expr* a, const Expr* b) {
    if (a == NULL || b == NULL)
        return false;

    if (a->type != b->type)
        return (EXPR_NUMBER_P(a) && EXPR_NUMBER_P(b))
                 ? (expr_get_generic_num(a) > expr_get_generic_num(b))
                 : false;

    switch (a->type) {
        case EXPR_NUM_INT:
            return a->val.n > b->val.n;

        case EXPR_NUM_FLT:
            return a->val.f > b->val.f;

        case EXPR_ERR:
        case EXPR_SYMBOL:
        case EXPR_STRING:
            return strcmp(a->val.s, b->val.s) > 0;

        case EXPR_PAIR:
        case EXPR_PRIM:
        case EXPR_LAMBDA:
        case EXPR_MACRO:
        case EXPR_UNKNOWN:
            return false;
    }

    __builtin_unreachable();
}

/*----------------------------------------------------------------------------*/

bool expr_is_proper_list(const Expr* e) {
    SL_ASSERT(e != NULL);

    /*
     * (defun expr-is-proper-list (e)
     *   (cond ((null? e) tru)
     *         ((pair? e) (expr-is-proper-list (cdr e)))
     *         (tru nil)))
     */
    while (!expr_is_nil(e)) {
        if (e->type != EXPR_PAIR)
            return false;
        e = CDR(e);
    }

    return true;
}

size_t expr_list_len(const Expr* e) {
    SL_ASSERT(e != NULL);
    SL_ASSERT(expr_is_proper_list(e));

    size_t result = 0;
    for (; !expr_is_nil(e); e = CDR(e))
        result++;
    return result;
}

Expr* expr_list_nth(const Expr* e, size_t n) {
    SL_ASSERT(e != NULL);
    SL_ASSERT(n > 0 && n <= expr_list_len(e));

    while (--n > 0)
        e = CDR(e);

    return CAR(e);
}

bool expr_is_member(const Expr* lst, const Expr* e) {
    SL_ASSERT(lst != NULL && e != NULL);
    SL_ASSERT(expr_is_proper_list(lst));

    for (; !expr_is_nil(lst); lst = CDR(lst))
        if (expr_equal(CAR(lst), e))
            return true;

    return false;
}

bool expr_list_is_homogeneous(const Expr* e) {
    SL_ASSERT(e != NULL);
    SL_ASSERT(expr_is_proper_list(e));

    /*
     * Store the type of the first element, and start checking from the second
     * one.
     */
    const enum EExprType first_type = CAR(e)->type;
    for (e = CDR(e); !expr_is_nil(e); e = CDR(e))
        if (CAR(e)->type != first_type)
            return false;

    return true;
}

bool expr_list_has_type(const Expr* e, enum EExprType type) {
    SL_ASSERT(e != NULL);
    SL_ASSERT(expr_is_proper_list(e));

    for (; !expr_is_nil(e); e = CDR(e))
        if (CAR(e)->type == type)
            return true;

    return false;
}

bool expr_list_has_only_numbers(const Expr* e) {
    SL_ASSERT(e != NULL);
    SL_ASSERT(expr_is_proper_list(e));

    for (; !expr_is_nil(e); e = CDR(e))
        if (!EXPR_NUMBER_P(CAR(e)))
            return false;

    return true;
}

bool expr_list_has_only_lists(const Expr* e) {
    SL_ASSERT(e != NULL);
    SL_ASSERT(expr_is_proper_list(e));

    for (; !expr_is_nil(e); e = CDR(e))
        if (!expr_is_proper_list(CAR(e)))
            return false;

    return true;
}

/*----------------------------------------------------------------------------*/

/*
 * Print each element of a list to the specified file using the specified
 * 'print_func'. The argument doesn't have to be a proper list.
 */
static void expr_list_print(FILE* fp, const Expr* list,
                            void (*print_func)(FILE* fp, const Expr* e)) {
    SL_ASSERT(EXPR_PAIR_P(list));

    for (;;) {
        print_func(fp, CAR(list));

        list = CDR(list);
        if (expr_is_nil(list))
            break;

        fputc(' ', fp);

        if (!EXPR_PAIR_P(list)) {
            fprintf(fp, ". ");
            print_func(fp, list);
            break;
        }
    }
}

void expr_print(FILE* fp, const Expr* e) {
    if (e == NULL) {
        SL_ERR("Unexpected NULL expression. Returning...");
        return;
    }

    switch (e->type) {
        case EXPR_NUM_INT:
            fprintf(fp, "%lld", e->val.n);
            break;

        case EXPR_NUM_FLT:
            fprintf(fp, "%f", e->val.f);
            break;

        case EXPR_SYMBOL:
            fprintf(fp, "%s", e->val.s);
            break;

        case EXPR_STRING:
            print_escaped_str(fp, e->val.s);
            break;

        case EXPR_ERR:
            err_print(stderr, e);
            break;

        case EXPR_PAIR:
            fputc('(', fp);
            expr_list_print(fp, e, expr_print);
            fputc(')', fp);
            break;

        case EXPR_PRIM:
            fprintf(fp, "<primitive %p>", e->val.prim);
            break;

        case EXPR_LAMBDA:
            fprintf(fp, "<lambda>");
            break;

        case EXPR_MACRO:
            fprintf(fp, "<macro>");
            break;

        case EXPR_UNKNOWN:
            fprintf(fp, "<unknown>");
            break;
    }
}

bool expr_write(FILE* fp, const Expr* e) {
    SL_ASSERT(e != NULL);

    switch (e->type) {
        case EXPR_NUM_INT:
            fprintf(fp, "%lld", e->val.n);
            break;

        case EXPR_NUM_FLT:
            fprintf(fp, "%f", e->val.f);
            break;

        case EXPR_SYMBOL:
            fprintf(fp, "%s", e->val.s);
            break;

        case EXPR_STRING:
            print_escaped_str(fp, e->val.s);
            break;

        case EXPR_PAIR:
            fputc('(', fp);
            expr_list_print(fp, e, expr_print);
            fputc(')', fp);
            break;

        case EXPR_LAMBDA:
        case EXPR_MACRO:
            fprintf(fp,
                    "(%s ",
                    (e->type == EXPR_LAMBDA)  ? "lambda"
                    : (e->type == EXPR_MACRO) ? "macro"
                                              : "ERROR");
            lambdactx_print_args(fp, e->val.lambda);
            fputc(' ', fp);
            expr_list_print(fp, e->val.lambda->body, expr_print);
            fputc(')', fp);
            break;

        case EXPR_ERR:
        case EXPR_PRIM:
        case EXPR_UNKNOWN:
            return false;
    }

    return true;
}

#define INDENT_STEP 4
void expr_print_debug(FILE* fp, const Expr* e) {
    static int indent = 0;

    for (int i = 0; i < indent; i++)
        fputc(' ', fp);

    if (e == NULL) {
        SL_ERR("Unexpected NULL expression. Returning...");
        return;
    }

    switch (e->type) {
        case EXPR_NUM_INT: {
            fprintf(fp, "[INT] %lld\n", e->val.n);
        } break;

        case EXPR_NUM_FLT: {
            fprintf(fp, "[FLT] %f\n", e->val.f);
        } break;

        case EXPR_ERR: {
            fprintf(fp, "[ERR] \"%s\"\n", e->val.s);
        } break;

        case EXPR_SYMBOL: {
            fprintf(fp, "[SYM] \"%s\"\n", e->val.s);
        } break;

        case EXPR_STRING: {
            fprintf(fp, "[STR] ");
            print_escaped_str(fp, e->val.s);
            fputc('\n', fp);
        } break;

        case EXPR_PAIR: {
            fprintf(fp, "[PAI]\n");

            /* If the token is a pair, print CAR and CDR */
            indent += INDENT_STEP;
            expr_print_debug(fp, CAR(e));
            expr_print_debug(fp, CDR(e));
            indent -= INDENT_STEP;
        } break;

        case EXPR_PRIM: {
            fprintf(fp, "[PRI] <primitive %p>\n", e->val.prim);
        } break;

        case EXPR_MACRO:
        case EXPR_LAMBDA: {
            fprintf(fp,
                    "[FUN] <%s>\n",
                    (e->type == EXPR_LAMBDA) ? "lambda" : "macro");

            /* Print list of formal arguments */
            indent += INDENT_STEP;
            for (int i = 0; i < indent; i++)
                fputc(' ', fp);
            fprintf(fp, "Formals: ");
            lambdactx_print_args(fp, e->val.lambda);
            fputc('\n', fp);

            /* Print each expression in the body of the function */
            for (int i = 0; i < indent; i++)
                putchar(' ');
            fprintf(fp, "Body:\n");
            indent += INDENT_STEP;
            expr_print_debug(fp, e->val.lambda->body);
            indent -= INDENT_STEP * 2;
        } break;

        case EXPR_UNKNOWN: {
            fprintf(fp, "[UNK] (Stopping)");
            return;
        }
    }
}
