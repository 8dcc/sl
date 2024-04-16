
#ifndef EVAL_H_
#define EVAL_H_ 1

#include "env.h"
#include "expr.h"

/* Evaluate expression recursively, asociating symbols to their values in `env'.
 * If an S-expression is encountered, it will call `apply'.
 * No data from `e' is re-used, so it should be freed by the caller. */
Expr* eval(Env* env, Expr* e);

/* Call `func' with the remaining (evaluated) `args'. */
Expr* apply(Env* env, Expr* func, Expr* args);

#endif /* EVAL_H_ */
