
#ifndef EVAL_H_
#define EVAL_H_ 1

struct Env;  /* env.h */
struct Expr; /* expr.h */

/* Evaluate expression recursively, asociating symbols to their values in `env'.
 * If an S-expression is encountered, it will call `apply'.
 * No data from `e' is re-used, so it should be freed by the caller. */
struct Expr* eval(struct Env* env, struct Expr* e);

/* Call `func' with the remaining (evaluated) `args'. */
struct Expr* apply(struct Env* env, struct Expr* func, struct Expr* args);

#endif /* EVAL_H_ */
