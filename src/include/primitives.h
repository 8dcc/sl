
#ifndef PRIMITIVES_H_
#define PRIMITIVES_H_ 1

#include "parser.h"

#define DECLARE_PRIM(NAME) Expr* prim_##NAME(Expr*)

DECLARE_PRIM(add);
DECLARE_PRIM(sub);
DECLARE_PRIM(mul);
DECLARE_PRIM(div);

#endif /* PRIMITIVES_H_ */
