
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/expr.h"
#include "include/env.h"
#include "include/util.h"

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
            /* Free each component, allocated in prim_lambda().
             * First, the environment and the body of the lambda */
            env_free(root->val.lambda->env);
            expr_free(root->val.lambda->body);

            /* Free each formal argument string, and the array itself */
            for (size_t i = 0; i < root->val.lambda->formals_num; i++)
                free(root->val.lambda->formals[i]);
            free(root->val.lambda->formals);

            /* And finally, the LambdaCtx structure itself */
            free(root->val.lambda);
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

    Expr* ret = sl_safe_malloc(sizeof(Expr));
    ret->type = e->type;

    switch (e->type) {
        case EXPR_SYMBOL:
            /* Allocate a new copy of the string */
            ret->val.s = strdup(e->val.s);
            break;
        case EXPR_CONST:
            ret->val.n = e->val.n;
            break;
        case EXPR_PARENT:
            ret->val.children = NULL;
            break;
        case EXPR_PRIM:
            ret->val.prim = e->val.prim;
            break;
        case EXPR_LAMBDA:
            ret->val.lambda       = sl_safe_malloc(sizeof(LambdaCtx));
            ret->val.lambda->env  = env_clone(e->val.lambda->env);
            ret->val.lambda->body = expr_clone_list(e->val.lambda->body);

            /* Allocate a new string array for the formals, and copy them */
            ret->val.lambda->formals_num = e->val.lambda->formals_num;
            ret->val.lambda->formals =
              sl_safe_malloc(ret->val.lambda->formals_num * sizeof(char*));
            for (size_t i = 0; i < ret->val.lambda->formals_num; i++)
                ret->val.lambda->formals[i] = strdup(e->val.lambda->formals[i]);

            break;
        case EXPR_ERR:
            ERR("Trying to clone <error>");
            break;
    }

    /* NOTE: We don't copy pointers like e->next or e->val.children  */
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
void expr_print_debug(Expr* e) {
    static int indent = 0;

    /* This function shouldn't be called with NULL */
    if (e == NULL) {
        printf("[ERR] ");
        ERR("Got NULL as argument. Aborting...");
        return;
    }

    for (int i = 0; i < indent; i++)
        putchar(' ');

    switch (e->type) {
        case EXPR_CONST:
            printf("[NUM] %f\n", e->val.n);
            break;
        case EXPR_SYMBOL:
            printf("[SYM] \"%s\"\n", e->val.s);
            break;
        case EXPR_PARENT:
            printf("[LST]");

            /* List with no children: NIL */
            if (e->val.children == NULL) {
                printf(" (NIL)\n");
                break;
            }

            putchar('\n');

            /* If the token is a parent, indent and print all children */
            indent += INDENT_STEP;
            expr_print_debug(e->val.children);
            indent -= INDENT_STEP;
            break;
        case EXPR_PRIM:
            printf("[PRI] <primitive %p>\n", e->val.prim);
            break;
        case EXPR_LAMBDA:
            printf("[FUN] <lambda>\n");

            /* Print list of formal arguments */
            indent += INDENT_STEP;
            for (int i = 0; i < indent; i++)
                putchar(' ');

            printf("Formals: (");
            for (size_t i = 0; i < e->val.lambda->formals_num; i++) {
                if (i > 0)
                    putchar(' ');
                printf("%s", e->val.lambda->formals[i]);
            }
            printf(")\n");

            /* Print each expression in the body of the function */
            expr_print_debug(e->val.lambda->body);
            indent -= INDENT_STEP;
            break;
        case EXPR_ERR:
            printf("[ERR] (Stopping)");
            return;
    }

    if (e->next != NULL)
        expr_print_debug(e->next);
}

static void print_sexpr(Expr* e) {
    putchar('(');
    for (Expr* cur = e->val.children; cur != NULL; cur = cur->next) {
        expr_print(cur);

        if (cur->next != NULL)
            putchar(' ');
    }
    putchar(')');
}

void expr_print(Expr* e) {
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
            /* List with no children: NIL */
            if (e->val.children == NULL)
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

void expr_println(Expr* e) {
    expr_print(e);
    putchar('\n');
}

/*----------------------------------------------------------------------------*/

size_t expr_list_len(Expr* e) {
    size_t result = 0;

    for (; e != NULL; e = e->next)
        result++;

    return result;
}
