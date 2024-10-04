
#ifndef PRIMITIVES_H_
#define PRIMITIVES_H_ 1

struct Env;  /* env.h */
struct Expr; /* expr.h */

#define DECLARE_PRIM(NAME) struct Expr* prim_##NAME(struct Env*, struct Expr*)

/* Special Form (prim_special.c) */
DECLARE_PRIM(quote);
DECLARE_PRIM(define);
DECLARE_PRIM(define_global);
DECLARE_PRIM(lambda);
DECLARE_PRIM(macro);
DECLARE_PRIM(begin);
DECLARE_PRIM(if);
DECLARE_PRIM(or);
DECLARE_PRIM(and);

/* General (prim_general.c) */
DECLARE_PRIM(eval);
DECLARE_PRIM(apply);
DECLARE_PRIM(macroexpand);
DECLARE_PRIM(random);
DECLARE_PRIM(set_random_seed);

/* Logical (prim_logic.c) */
DECLARE_PRIM(equal);
DECLARE_PRIM(lt);
DECLARE_PRIM(gt);

/* Type-checking (prim_type.c) */
DECLARE_PRIM(type_of);
DECLARE_PRIM(is_int);
DECLARE_PRIM(is_flt);
DECLARE_PRIM(is_symbol);
DECLARE_PRIM(is_string);
DECLARE_PRIM(is_list);
DECLARE_PRIM(is_primitive);
DECLARE_PRIM(is_lambda);
DECLARE_PRIM(is_macro);

/* Type-conversion (prim_type.c) */
DECLARE_PRIM(int2flt);
DECLARE_PRIM(flt2int);
DECLARE_PRIM(int2str);
DECLARE_PRIM(flt2str);
DECLARE_PRIM(str2int);
DECLARE_PRIM(str2flt);

/* List-related (prim_list.c) */
DECLARE_PRIM(list);
DECLARE_PRIM(cons);
DECLARE_PRIM(car);
DECLARE_PRIM(cdr);
DECLARE_PRIM(length);
DECLARE_PRIM(append);

/* String-related (prim_string.c) */
DECLARE_PRIM(write_to_string);
DECLARE_PRIM(format);
DECLARE_PRIM(substring);
DECLARE_PRIM(string_matches);

/* Arithmetic (prim_arith.c) */
DECLARE_PRIM(add);
DECLARE_PRIM(sub);
DECLARE_PRIM(mul);
DECLARE_PRIM(div);
DECLARE_PRIM(mod);
DECLARE_PRIM(quotient);
DECLARE_PRIM(remainder);
DECLARE_PRIM(floor);

/* Bit-wise (prim_bitwise.c) */
DECLARE_PRIM(bit_and);
DECLARE_PRIM(bit_or);
DECLARE_PRIM(bit_xor);
DECLARE_PRIM(bit_not);
DECLARE_PRIM(shr);
DECLARE_PRIM(shl);

/* Input/Output (prim_io.c) */
DECLARE_PRIM(read);
DECLARE_PRIM(write);
DECLARE_PRIM(read_str);
DECLARE_PRIM(print_str);
DECLARE_PRIM(error);

#endif /* PRIMITIVES_H_ */
