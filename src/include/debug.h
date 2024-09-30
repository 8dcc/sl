
#ifndef DEBUG_H_
#define DEBUG_H_ 1

#include <stdbool.h>

struct Env;  /* env.h */
struct Expr; /* expr.h */

/*
 * Is the function `e' being traced in the `*debug-trace*' Lisp variable?
 */
bool debug_is_traced_function(const struct Env* env, const struct Expr* e);

/*
 * Print the opening and closing of a function trace.
 */
void debug_trace_print_pre(const struct Expr* e);
void debug_trace_print_post(const struct Expr* e);

#endif /* DEBUG_H_ */
