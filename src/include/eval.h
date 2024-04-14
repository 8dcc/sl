
#ifndef EVAL_H_
#define EVAL_H_ 1

#include "expr.h"

/* Evaluate expression recursively. No data from `e' is re-used, so it should be
 * freed by the caller. */
Expr* eval_expr(Expr* e);

/* Evaluate an S-Expression (along with its sub-expressions) as a function
 * call. */
Expr* eval_sexpr(Expr* e);

#endif /* EVAL_H_ */
