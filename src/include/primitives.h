
#ifndef PRIMITIVES_H_
#define PRIMITIVES_H_ 1

struct Env;  /* env.h */
struct Expr; /* expr.h */

#define DECLARE_PRIM(NAME) struct Expr* prim_##NAME(struct Env*, struct Expr*)

/* Special Form primitives */
DECLARE_PRIM(quote);
DECLARE_PRIM(define);
DECLARE_PRIM(lambda);
DECLARE_PRIM(macro);
DECLARE_PRIM(begin);
DECLARE_PRIM(if);
DECLARE_PRIM(or);
DECLARE_PRIM(and);

/* General primitives */
DECLARE_PRIM(eval);
DECLARE_PRIM(apply);
DECLARE_PRIM(macroexpand);
DECLARE_PRIM(random);
DECLARE_PRIM(set_random_seed);

/* List-related primitives */
DECLARE_PRIM(list);
DECLARE_PRIM(cons);
DECLARE_PRIM(car);
DECLARE_PRIM(cdr);
DECLARE_PRIM(append);

/* String-related primitives */
DECLARE_PRIM(concat);

/* Arithmetic primitives */
DECLARE_PRIM(add);
DECLARE_PRIM(sub);
DECLARE_PRIM(mul);
DECLARE_PRIM(div);
DECLARE_PRIM(mod);
DECLARE_PRIM(quotient);
DECLARE_PRIM(remainder);
DECLARE_PRIM(floor);

/* Type-conversion and type-checking primitives */
DECLARE_PRIM(int2flt);
DECLARE_PRIM(flt2int);
DECLARE_PRIM(is_int);
DECLARE_PRIM(is_flt);
DECLARE_PRIM(is_symbol);
DECLARE_PRIM(is_string);
DECLARE_PRIM(is_list);
DECLARE_PRIM(is_primitive);
DECLARE_PRIM(is_lambda);
DECLARE_PRIM(is_macro);

/* Bit-wise primitives */
DECLARE_PRIM(bit_and);
DECLARE_PRIM(bit_or);
DECLARE_PRIM(bit_xor);
DECLARE_PRIM(bit_not);
DECLARE_PRIM(shr);
DECLARE_PRIM(shl);

/* Logical primitives */
DECLARE_PRIM(equal);
DECLARE_PRIM(lt);
DECLARE_PRIM(gt);

#endif /* PRIMITIVES_H_ */
