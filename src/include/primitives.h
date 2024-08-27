
#ifndef PRIMITIVES_H_
#define PRIMITIVES_H_ 1

struct Env;  /* env.h */
struct Expr; /* expr.h */

#define DECLARE_PRIM(NAME) struct Expr* prim_##NAME(struct Env*, struct Expr*)

/* Special Form primitives */
DECLARE_PRIM(quote);
DECLARE_PRIM(define);
DECLARE_PRIM(lambda);
DECLARE_PRIM(if);

/* General primitives */
DECLARE_PRIM(eval);
DECLARE_PRIM(apply);
DECLARE_PRIM(begin);

/* List-related primitives */
DECLARE_PRIM(cons);
DECLARE_PRIM(car);
DECLARE_PRIM(cdr);

/* Arithmetic primitives */
DECLARE_PRIM(add);
DECLARE_PRIM(sub);
DECLARE_PRIM(mul);
DECLARE_PRIM(div);

/* Logical primitives */
DECLARE_PRIM(equal);

#endif /* PRIMITIVES_H_ */
