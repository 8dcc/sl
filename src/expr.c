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

#include "include/expr.h"
#include "include/env.h"
#include "include/lambda.h"
#include "include/util.h"

Expr* expr_new(enum EExprType type) {
    Expr* ret         = sl_safe_malloc(sizeof(Expr));
    ret->type         = type;
    ret->val.children = NULL;
    ret->next         = NULL;
    return ret;
}

void expr_free(Expr* e) {
    if (e == NULL)
        return;

    /*
     * First, check if the value of the current expression was allocated, and
     * free it.  Then, free the `Expr' structure itself, usually allocated in
     * `expr_new'.
     */
    switch (e->type) {
        case EXPR_PARENT:
            expr_list_free(e->val.children);
            break;

        case EXPR_ERR:
        case EXPR_SYMBOL:
        case EXPR_STRING:
            free(e->val.s);
            break;

        case EXPR_LAMBDA:
        case EXPR_MACRO:
            lambda_ctx_free(e->val.lambda);
            break;

        case EXPR_UNKNOWN:
        case EXPR_NUM_INT:
        case EXPR_NUM_FLT:
        case EXPR_PRIM:
            break;
    }

    free(e);
}

void expr_list_free(Expr* e) {
    while (e != NULL) {
        Expr* next = e->next;
        expr_free(e);
        e = next;
    }
}

/*----------------------------------------------------------------------------*/

Expr* expr_clone(const Expr* e) {
    if (e == NULL)
        return NULL;

    /*
     * Nothing allocated (e.g. symbol strings) is re-used from the old
     * expression, everything is allocated and copied again.
     *
     * Note that this function does NOT clone inferior or adjacent nodes. See
     * `expr_clone_recur' and `expr_clone_list' respectively.
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
            ret->val.s = sl_safe_strdup(e->val.s);
            break;

        case EXPR_PARENT:
            ret->val.children = NULL;
            break;

        case EXPR_PRIM:
            ret->val.prim = e->val.prim;
            break;

        case EXPR_MACRO:
        case EXPR_LAMBDA:
            ret->val.lambda = lambda_ctx_clone(e->val.lambda);
            break;

        case EXPR_UNKNOWN:
            SL_ERR("Trying to clone <unknown>");
            break;
    }

    ret->next = NULL;
    return ret;
}

Expr* expr_clone_recur(const Expr* e) {
    if (e == NULL)
        return NULL;

    Expr* cloned = expr_clone(e);

    if (e->type == EXPR_PARENT) {
        /*
         * The copy of the first child will be stored in `dummy_copy.next'. This
         * will be stored in `val.children' below.
         */
        Expr dummy_copy;
        dummy_copy.next = NULL;
        Expr* cur_copy  = &dummy_copy;

        for (Expr* cur = e->val.children; cur != NULL; cur = cur->next) {
            cur_copy->next = expr_clone_recur(cur);
            cur_copy       = cur_copy->next;
        }

        cloned->val.children = dummy_copy.next;
    }

    return cloned;
}

Expr* expr_list_clone(const Expr* e) {
    Expr dummy_copy;
    dummy_copy.next = NULL;
    Expr* cur_copy  = &dummy_copy;

    for (; e != NULL; e = e->next) {
        cur_copy->next = expr_clone_recur(e);
        cur_copy       = cur_copy->next;
    }

    return dummy_copy.next;
}

/*----------------------------------------------------------------------------*/

bool expr_is_nil(const Expr* e) {
    return e != NULL && ((e->type == EXPR_PARENT && e->val.children == NULL) ||
                         (e->type == EXPR_SYMBOL && e->val.s != NULL &&
                          strcmp(e->val.s, "nil") == 0));
}

bool expr_list_equal(const Expr* a, const Expr* b) {
    /*
     * Keep iterating while both nodes are equal. We check this by
     * calling `expr_equal', which allows NULL arguments.
     *
     * Since inside the loop both items are equal, if one of them is
     * NULL, it means we reached the end of both lists and they are
     * equal.
     *
     * This whole loop is similar to an implementation of strcmp().
     */
    while (expr_equal(a, b)) {
        if (a == NULL)
            return true;

        a = a->next;
        b = b->next;
    }

    /* If we broke out of the loop, an expression didn't match */
    return false;
}

bool expr_equal(const Expr* a, const Expr* b) {
    /*
     * If one of them is NULL, they are equal if the other is also NULL. This is
     * an important check, specially since when calling ourselves recursively.
     */
    if (a == NULL || b == NULL)
        return a == b;

    /*
     * If both of them are `nil', they are equal. This check is important
     * because the symbol "nil" and the empty list have different types but are
     * equal.
     */
    if (expr_is_nil(a) && expr_is_nil(b))
        return true;

    /*
     * This function checks for isomorphism, so if they don't share the same
     * type, they are not equal. For comparing numbers, see `prim_equal_num' or
     * `expr_lt'.
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

        case EXPR_PARENT:
            return expr_list_equal(a->val.children, b->val.children);

        case EXPR_PRIM:
            return a->val.prim == b->val.prim;

        case EXPR_MACRO:
        case EXPR_LAMBDA:
            return lambda_ctx_equal(a->val.lambda, b->val.lambda);

        case EXPR_UNKNOWN:
            return false;
    }

    __builtin_unreachable();
}

bool expr_lt(const Expr* a, const Expr* b) {
    if (a == NULL || b == NULL)
        return false;

    /* See `prim_equal_num' */
    if (a->type != b->type)
        return (EXPRP_NUMBER(a) && EXPRP_NUMBER(b))
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

        case EXPR_PARENT:
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
        return (EXPRP_NUMBER(a) && EXPRP_NUMBER(b))
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

        case EXPR_PARENT:
        case EXPR_PRIM:
        case EXPR_LAMBDA:
        case EXPR_MACRO:
        case EXPR_UNKNOWN:
            return false;
    }

    __builtin_unreachable();
}

/*----------------------------------------------------------------------------*/

size_t expr_list_len(const Expr* e) {
    size_t result = 0;

    for (; e != NULL; e = e->next)
        result++;

    return result;
}

bool expr_list_is_member(const Expr* lst, const Expr* e) {
    if (e == NULL)
        return false;

    for (; lst != NULL; lst = lst->next)
        if (expr_equal(lst, e))
            return true;

    return false;
}

bool expr_list_is_homogeneous(const Expr* e) {
    if (e == NULL)
        return false;

    const enum EExprType first_type = e->type;
    for (e = e->next; e != NULL; e = e->next)
        if (e->type != first_type)
            return false;

    return true;
}

bool expr_list_has_type(const Expr* e, enum EExprType type) {
    for (; e != NULL; e = e->next)
        if (e->type == type)
            return true;

    return false;
}

bool expr_list_has_only_numbers(const Expr* e) {
    if (e == NULL)
        return false;

    for (; e != NULL; e = e->next)
        if (!EXPRP_NUMBER(e))
            return false;

    return true;
}

/*----------------------------------------------------------------------------*/

void expr_list_print(FILE* fp, const Expr* e) {
    fputc('(', fp);
    for (; e != NULL; e = e->next) {
        expr_print(fp, e);

        if (e->next != NULL)
            fputc(' ', fp);
    }
    fputc(')', fp);
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
            /*
             * TODO: Print with color if SL_NO_COLOR is not defined, similar to
             * `sl_print_err'.
             */
            fprintf(fp, "Error: %s", e->val.s);
            break;

        case EXPR_PARENT:
            if (expr_is_nil(e))
                fprintf(fp, "nil");
            else
                expr_list_print(fp, e->val.children);
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

static bool expr_list_write(FILE* fp, const Expr* e) {
    for (; e != NULL; e = e->next) {
        if (!expr_write(fp, e))
            return false;

        if (e->next != NULL)
            fputc(' ', fp);
    }
    return true;
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

        case EXPR_PARENT:
            if (expr_is_nil(e)) {
                fprintf(fp, "nil");
            } else {
                fputc('(', fp);
                expr_list_write(fp, e->val.children);
                fputc(')', fp);
            }
            break;

        case EXPR_LAMBDA:
        case EXPR_MACRO:
            fprintf(fp, "(%s ",
                    (e->type == EXPR_LAMBDA)  ? "lambda"
                    : (e->type == EXPR_MACRO) ? "macro"
                                              : "ERROR");
            lambda_ctx_print_args(fp, e->val.lambda);
            fputc(' ', fp);
            expr_list_write(fp, e->val.lambda->body);
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

        case EXPR_PARENT: {
            fprintf(fp, "[LST]");

            if (expr_is_nil(e)) {
                fprintf(fp, " (NIL)\n");
                break;
            }

            fputc('\n', fp);

            /* If the token is a parent, indent and print all children */
            indent += INDENT_STEP;
            expr_print_debug(fp, e->val.children);
            indent -= INDENT_STEP;
        } break;

        case EXPR_PRIM: {
            fprintf(fp, "[PRI] <primitive %p>\n", e->val.prim);
        } break;

        case EXPR_MACRO:
        case EXPR_LAMBDA: {
            fprintf(fp, "[FUN] <%s>\n",
                    (e->type == EXPR_LAMBDA) ? "lambda" : "macro");

            /* Print list of formal arguments */
            indent += INDENT_STEP;
            for (int i = 0; i < indent; i++)
                fputc(' ', fp);
            fprintf(fp, "Formals: ");
            lambda_ctx_print_args(fp, e->val.lambda);
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

    if (e->next != NULL)
        expr_print_debug(fp, e->next);
}
