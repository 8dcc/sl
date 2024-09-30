
#include <stdbool.h>
#include <stdio.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/debug.h"

static size_t trace_nesting = 0;

static void debug_trace_print_expr(const Expr* e) {
    for (size_t i = 0; i < trace_nesting; i++)
        printf("  ");

    printf("%zu: ", trace_nesting % 10);

    if (e == NULL)
        printf("ERR");
    else
        expr_print(e);

    putchar('\n');
}

bool debug_is_traced_function(const Env* env, const Expr* e) {
    Expr* debug_trace_list = env_get(env, "*debug-trace*");
    if (debug_trace_list == NULL || debug_trace_list->type != EXPR_PARENT)
        return false;

    const bool result = expr_list_is_member(debug_trace_list->val.children, e);
    expr_free(debug_trace_list);

    return result;
}

void debug_trace_print_pre(const Expr* e) {
    debug_trace_print_expr(e);
    trace_nesting++;
}

void debug_trace_print_post(const Expr* e) {
    trace_nesting--;
    debug_trace_print_expr(e);
}
