
#ifndef PRIMITIVES_H_
#define PRIMITIVES_H_ 1

#include "parser.h"

typedef Expr* (*PrimitiveFuncPtr)(Expr*);

typedef struct {
    const char* s;
    PrimitiveFuncPtr f;
} PrimitiveFuncPair;

/* Symbols associated to C functions (primitives) are handled in eval.c instead
 * of being associated in the environment. */
extern PrimitiveFuncPair* primitives;
extern PrimitiveFuncPair* non_eval_primitives;

/*----------------------------------------------------------------------------*/

#define DECLARE_PRIM(NAME) Expr* prim_##NAME(Expr*)

DECLARE_PRIM(quote);
DECLARE_PRIM(add);
DECLARE_PRIM(sub);
DECLARE_PRIM(mul);
DECLARE_PRIM(div);

#endif /* PRIMITIVES_H_ */
