
#ifndef EVAL_H_
#define EVAL_H_ 1

struct Env;  /* env.h */
struct Expr; /* expr.h */

/*
 * Evaluate expression recursively.
 *
 * Symbols return their associated value from `env'. Lists are treated as
 * function applications using `apply'.
 *
 * No data from `e' is re-used, so it should be freed by the caller.
 */
struct Expr* eval(struct Env* env, struct Expr* e);

/*
 * Call `func' with the specified `args'.
 *
 * The `env' and `func' pointers should not be NULL, and `func' should be an
 * "applicable" expression as specified by `expr_is_applicable'.
 *
 * The arguments are passed to the function unchanged, so the evaluation is up
 * to the caller.
 */
struct Expr* apply(struct Env* env, struct Expr* func, struct Expr* args);

#endif /* EVAL_H_ */
