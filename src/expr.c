
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

void expr_free(Expr* root) {
    /* This function shouldn't be called with NULL */
    if (root == NULL)
        return;

    /* If the expression has an adjacent one, free that one first */
    if (root->next != NULL)
        expr_free(root->next);

    switch (root->type) {
        case EXPR_PARENT:
            /* If the expression has children, free them */
            if (root->val.children != NULL)
                expr_free(root->val.children);
            break;
        case EXPR_SYMBOL:
            /* Free the symbol string, allocated in tokens_scan() */
            free(root->val.s);
            break;
        case EXPR_LAMBDA:
            lambda_ctx_free(root->val.lambda);
            break;
        case EXPR_ERR:
        case EXPR_CONST:
        case EXPR_PRIM:
            break;
    }

    /* Free the expression itself, that we allocated on parse() */
    free(root);
}

/*----------------------------------------------------------------------------*/

Expr* expr_clone(const Expr* e) {
    if (e == NULL)
        return NULL;

    Expr* ret = expr_new(e->type);

    switch (e->type) {
        case EXPR_SYMBOL:
            /* Don't reuse the old pointer */
            ret->val.s = sl_safe_strdup(e->val.s);
            break;

        case EXPR_CONST:
            ret->val.n = e->val.n;
            break;

        case EXPR_PARENT:
            /* See `expr_clone_recur' for recursive cloning */
            ret->val.children = NULL;
            break;

        case EXPR_PRIM:
            /* Copy the C function pointer */
            ret->val.prim = e->val.prim;
            break;

        case EXPR_LAMBDA: {
            ret->val.lambda = lambda_ctx_clone(e->val.lambda);
        } break;

        case EXPR_ERR:
            ERR("Trying to clone <error>");
            break;
    }

    /* We don't copy inferior or adjacent nodes. See, `expr_clone_recur' and
     * `expr_clone_list' respectively. */
    ret->next = NULL;
    return ret;
}

Expr* expr_clone_recur(const Expr* e) {
    if (e == NULL)
        return NULL;

    Expr* cloned = expr_clone(e);

    /* If the expression we just cloned is a parent */
    if (e->type == EXPR_PARENT) {
        /* The copy of the first child will be stored in dummy_copy.next */
        Expr dummy_copy;
        dummy_copy.next = NULL;
        Expr* cur_copy  = &dummy_copy;

        /* Clone all children, while linking them together in the new list. This
         * is similar to how we evaluate function arguments in `eval' */
        for (Expr* cur = e->val.children; cur != NULL; cur = cur->next) {
            /* Clone each children recursively */
            cur_copy->next = expr_clone_recur(cur);

            /* Move to the next argument in our copy list */
            cur_copy = cur_copy->next;
        }

        /* See comment above */
        cloned->val.children = dummy_copy.next;
    }

    return cloned;
}

Expr* expr_clone_list(const Expr* e) {
    Expr dummy_copy;
    dummy_copy.next = NULL;
    Expr* cur_copy  = &dummy_copy;

    for (const Expr* cur = e; cur != NULL; cur = cur->next) {
        cur_copy->next = expr_clone_recur(cur);
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

    /* This function shouldn't be called with NULL */
    if (e == NULL) {
        printf("[ERR] ");
        ERR("Got NULL as argument. Returning...");
        return;
    }

    switch (e->type) {
        case EXPR_CONST: {
            printf("[NUM] %f\n", e->val.n);
        } break;

        case EXPR_SYMBOL: {
            printf("[SYM] \"%s\"\n", e->val.s);
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

        case EXPR_LAMBDA: {
            printf("[FUN] <lambda>\n");

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

static void print_sexpr(const Expr* e) {
    putchar('(');
    for (Expr* cur = e->val.children; cur != NULL; cur = cur->next) {
        expr_print(cur);

        if (cur->next != NULL)
            putchar(' ');
    }
    putchar(')');
}

void expr_print(const Expr* e) {
    /* This function shouldn't be called with NULL */
    if (e == NULL) {
        ERR("Got null expression.");
        return;
    }

    switch (e->type) {
        case EXPR_CONST:
            printf("%f", e->val.n);
            break;

        case EXPR_SYMBOL:
            printf("%s", e->val.s);
            break;

        case EXPR_PARENT:
            if (expr_is_nil(e))
                printf("nil");
            else
                print_sexpr(e);
            break;

        case EXPR_PRIM:
            printf("<primitive %p>", e->val.prim);
            break;

        case EXPR_LAMBDA:
            printf("<lambda>");
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
        case EXPR_CONST:
            /* Compare the decimal values */
            return a->val.n == b->val.n;

        case EXPR_SYMBOL:
            /* Compare the symbol strings */
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
            /* Compare the C pointers for the primitive functions */
            return a->val.prim == b->val.prim;

        case EXPR_LAMBDA:
            /* TODO: Compare lambda bodies and parameters */
            return false;

        case EXPR_ERR:
            return false;
    }

    return true;
}
