/*
 * Copyright 2024 8dcc
 *
 * This file is part of SL.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * SL. If not, see <https://www.gnu.org/licenses/>.
 */

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
 * "applicable" expression as specified by `EXPRP_APPLICABLE'.
 *
 * The arguments are passed to the function unchanged, so the evaluation is up
 * to the caller.
 */
struct Expr* apply(struct Env* env, struct Expr* func, struct Expr* args);

#endif /* EVAL_H_ */
