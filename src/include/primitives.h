
#ifndef PRIMITIVES_H_
#define PRIMITIVES_H_ 1

struct Env;  /* env.h */
struct Expr; /* expr.h */

#define DECLARE_PRIM(NAME) struct Expr* prim_##NAME(struct Env*, struct Expr*)

DECLARE_PRIM(quote);
DECLARE_PRIM(define);
DECLARE_PRIM(lambda);
DECLARE_PRIM(eval);
DECLARE_PRIM(apply);
DECLARE_PRIM(begin);
DECLARE_PRIM(cons);
DECLARE_PRIM(car);
DECLARE_PRIM(cdr);
DECLARE_PRIM(add);
DECLARE_PRIM(sub);
DECLARE_PRIM(mul);
DECLARE_PRIM(div);

#endif /* PRIMITIVES_H_ */
