
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

        case EXPR_SYMBOL:
        case EXPR_STRING:
            free(e->val.s);
            break;

        case EXPR_LAMBDA:
        case EXPR_MACRO:
            lambda_ctx_free(e->val.lambda);
            break;

        case EXPR_ERR:
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

        case EXPR_ERR:
            ERR("Trying to clone <error>");
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

#define INDENT_STEP 4
void expr_print_debug(const Expr* e) {
    static int indent = 0;

    for (int i = 0; i < indent; i++)
        putchar(' ');

    if (e == NULL) {
        printf("[ERR] ");
        ERR("Got NULL as argument. Returning...");
        return;
    }

    switch (e->type) {
        case EXPR_NUM_INT: {
            printf("[INT] %lld\n", e->val.n);
        } break;

        case EXPR_NUM_FLT: {
            printf("[FLT] %f\n", e->val.f);
        } break;

        case EXPR_SYMBOL: {
            printf("[SYM] \"%s\"\n", e->val.s);
        } break;

        case EXPR_STRING: {
            printf("[STR] ");
            print_escaped_str(e->val.s);
            putchar('\n');
        } break;

        case EXPR_PARENT: {
            printf("[LST]");

            if (expr_is_nil(e)) {
                printf(" (NIL)\n");
                break;
            }

            putchar('\n');

            /* If the token is a parent, indent and print all children */
            indent += INDENT_STEP;
            expr_print_debug(e->val.children);
            indent -= INDENT_STEP;
        } break;

        case EXPR_PRIM: {
            printf("[PRI] <primitive %p>\n", e->val.prim);
        } break;

        case EXPR_MACRO:
        case EXPR_LAMBDA: {
            printf("[FUN] <%s>\n",
                   (e->type == EXPR_LAMBDA) ? "lambda" : "macro");

            /* Print list of formal arguments */
            indent += INDENT_STEP;
            for (int i = 0; i < indent; i++)
                putchar(' ');
            printf("Formals: ");
            lambda_ctx_print_args(e->val.lambda);
            putchar('\n');

            /* Print each expression in the body of the function */
            expr_print_debug(e->val.lambda->body);
            indent -= INDENT_STEP;
        } break;

        case EXPR_ERR: {
            printf("[ERR] (Stopping)");
            return;
        }
    }

    if (e->next != NULL)
        expr_print_debug(e->next);
}

static void print_list(const Expr* e) {
    putchar('(');
    for (; e != NULL; e = e->next) {
        expr_print(e);

        if (e->next != NULL)
            putchar(' ');
    }
    putchar(')');
}

void expr_print(const Expr* e) {
    if (e == NULL) {
        ERR("Got null expression.");
        return;
    }

    switch (e->type) {
        case EXPR_NUM_INT:
            printf("%lld", e->val.n);
            break;

        case EXPR_NUM_FLT:
            printf("%f", e->val.f);
            break;

        case EXPR_SYMBOL:
            printf("%s", e->val.s);
            break;

        case EXPR_STRING:
            print_escaped_str(e->val.s);
            break;

        case EXPR_PARENT:
            if (expr_is_nil(e))
                printf("nil");
            else
                print_list(e->val.children);
            break;

        case EXPR_PRIM:
            printf("<primitive %p>", e->val.prim);
            break;

        case EXPR_LAMBDA:
            printf("<lambda>");
            break;

        case EXPR_MACRO:
            printf("<macro>");
            break;

        case EXPR_ERR:
            printf("<error>");
            break;
    }
}

void expr_println(const Expr* e) {
    expr_print(e);
    putchar('\n');
}

/*----------------------------------------------------------------------------*/

size_t expr_list_len(const Expr* e) {
    size_t result = 0;

    for (; e != NULL; e = e->next)
        result++;

    return result;
}

bool expr_list_has_type(const Expr* e, enum EExprType type) {
    for (; e != NULL; e = e->next)
        if (e->type == type)
            return true;

    return false;
}

bool expr_list_has_only_type(const Expr* e, enum EExprType type) {
    for (; e != NULL; e = e->next)
        if (e->type != type)
            return false;

    return true;
}

bool expr_list_has_only_numbers(const Expr* e) {
    for (; e != NULL; e = e->next)
        if (!expr_is_number(e))
            return false;

    return true;
}

/*----------------------------------------------------------------------------*/

bool expr_is_nil(const Expr* e) {
    return e != NULL && e->type == EXPR_PARENT && e->val.children == NULL;
}

bool expr_equal(const Expr* a, const Expr* b) {
    /*
     * If one of them is NULL, they are equal if the other is also NULL. This is
     * an important check, specially since when calling ourselves recursively.
     */
    if (a == NULL || b == NULL)
        return a == b;

    if (a->type != b->type)
        return false;

    switch (a->type) {
        case EXPR_NUM_INT:
            return a->val.n == b->val.n;

        case EXPR_NUM_FLT:
            return a->val.f == b->val.f;

        case EXPR_SYMBOL:
        case EXPR_STRING:
            return strcmp(a->val.s, b->val.s) == 0;

        case EXPR_PARENT: {
            Expr* child_a = a->val.children;
            Expr* child_b = b->val.children;

            /*
             * Keep iterating while both children are equal. We check this by
             * calling ourselves recursivelly.
             *
             * Since inside the loop both children are equal, if one of them is
             * NULL, it means we reached the end of both lists and they are
             * equal.
             *
             * This whole loop is similar an implementation of strcmp().
             */
            while (expr_equal(child_a, child_b)) {
                if (child_a == NULL)
                    return true;

                child_a = child_a->next;
                child_b = child_b->next;
            }

            /* If we broke out of the loop, a children didn't match */
            return false;
        }

        case EXPR_PRIM:
            return a->val.prim == b->val.prim;

        case EXPR_MACRO:
        case EXPR_LAMBDA:
            /* TODO: Compare lambda bodies and parameters */
            return false;

        case EXPR_ERR:
            return false;
    }

    return true;
}

bool expr_lt(const Expr* a, const Expr* b) {
    if (a == NULL || b == NULL || a->type != b->type)
        return false;

    switch (a->type) {
        case EXPR_NUM_INT:
            return a->val.n < b->val.n;

        case EXPR_NUM_FLT:
            return a->val.f < b->val.f;

        case EXPR_SYMBOL:
        case EXPR_STRING:
            return strcmp(a->val.s, b->val.s) < 0;

        case EXPR_PARENT:
        case EXPR_PRIM:
        case EXPR_LAMBDA:
        case EXPR_MACRO:
        case EXPR_ERR:
            return false;
    }

    return true;
}

bool expr_gt(const Expr* a, const Expr* b) {
    if (a == NULL || b == NULL || a->type != b->type)
        return false;

    switch (a->type) {
        case EXPR_NUM_INT:
            return a->val.n > b->val.n;

        case EXPR_NUM_FLT:
            return a->val.f > b->val.f;

        case EXPR_SYMBOL:
        case EXPR_STRING:
            return strcmp(a->val.s, b->val.s) > 0;

        case EXPR_PARENT:
        case EXPR_PRIM:
        case EXPR_LAMBDA:
        case EXPR_MACRO:
        case EXPR_ERR:
            return false;
    }

    return true;
}
