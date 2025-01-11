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
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GARBAGE_COLLECTION_H_
#define GARBAGE_COLLECTION_H_ 1

struct Env;  /* env.h */
struct Expr; /* expr.h */

/*
 * Unmark all nodes in 'g_expr_pool', declared in 'expr_pool.h'.
 */
void gc_unmark_all(void);

/*
 * Mark all expressions in the specified environment as currently used. This
 * function doesn't free or collect anything, use 'gc_collect' for that.
 * Multiple environments can be marked before collecting.
 *
 * This only marks the expressions directly in the specified environment, it
 * does not mark parent environments.
 */
void gc_mark_env(struct Env* env);

/*
 * Mark the specified expression as currently used, recursively. This function
 * doesn't free or collect anything, use 'gc_collect' for that. Multiple
 * expressions can be marked before collecting.
 */
void gc_mark_expr(struct Expr* expr);

/*
 * Collect all unmarked expressions (i.e. all nodes whose 'NODE_GCMARKED' flag
 * is not set) in 'g_expr_pool', declared in 'expr_pool.h'.
 *
 * This function is usually called after unmarking all nodes with
 * 'gc_unmark_all', and then marking the desired nodes with one or more calls to
 * functions like 'gc_mark_env'.
 */
void gc_collect(void);

#endif /* GARBAGE_COLLECTION_H_ */
