
#include <stdbool.h>
#include <stdio.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/debug.h"

static size_t trace_nesting = 0;

static void print_trace_number(void) {
    for (size_t i = 0; i <= trace_nesting; i++)
        printf("  ");

    printf("%zu: ", trace_nesting % 10);
}

bool debug_is_traced_function(const Env* env, const Expr* e) {
    Expr* debug_trace_list = env_get(env, "*debug-trace*");
    if (debug_trace_list == NULL || debug_trace_list->type != EXPR_PARENT)
        return false;

    const bool result = expr_list_is_member(debug_trace_list->val.children, e);
    expr_free(debug_trace_list);

    return result;
}

void debug_trace_print_pre(const Expr* func, const Expr* arg) {
    print_trace_number();

    putchar('(');
    expr_print(func);
    for (; arg != NULL; arg = arg->next) {
        putchar(' ');
        expr_print(arg);
    }
    printf(")\n");

    trace_nesting++;
}

void debug_trace_print_post(const Expr* e) {
    trace_nesting--;
    print_trace_number();

    if (e == NULL)
        printf("ERR");
    else
        expr_print(e);

    putchar('\n');
}
