
#ifndef EVAL_H_
#define EVAL_H_ 1

#include "env.h"
#include "expr.h"

/* Evaluate expression recursively, asociating symbols to the values in `env'.
 * No data from `e' is re-used, so it should be freed by the caller. */
Expr* eval_expr(Env* env, Expr* e);

/* Evaluate an S-Expression (along with its sub-expressions) as a function
 * call. */
Expr* eval_sexpr(Env* env, Expr* e);

#endif /* EVAL_H_ */
