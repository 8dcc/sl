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

#ifndef ENV_H_
#define ENV_H_ 1

#include <stdbool.h>
#include <stdio.h> /* FILE */

struct Expr; /* expr.h */

/*----------------------------------------------------------------------------*/

/*
 * Environment error codes, returned by functions like 'env_bind'. See also
 * 'env_strerror' below.
 */
enum EEnvErr {
    ENV_ERR_NONE = 0,
    ENV_ERR_CONST,
};

/*
 * Possible flags for each 'EnvBinding' structure. They can be OR'd together.
 */
enum EEnvBindingFlags {
    ENV_FLAG_NONE    = 0,
    ENV_FLAG_CONST   = (1 << 0),
    ENV_FLAG_SPECIAL = (1 << 1),
};

/*
 * An 'EnvBinding' structure is used to bind a symbol to its expression, with
 * some specified flags from the 'EEnvBindingFlags' enum.
 */
typedef struct EnvBinding EnvBinding;
struct EnvBinding {
    char* sym;
    struct Expr* val;
    enum EEnvBindingFlags flags;
};

/*
 * An environment is simply an array of 'EnvBinding' structures, and a parent
 * environment.
 */
typedef struct Env Env;
struct Env {
    Env* parent;
    size_t size;
    EnvBinding* bindings;
};

/*----------------------------------------------------------------------------*/

/*
 * Globals, initialized (if necessary) on 'env_init_defaults'.
 *
 *   - nil: Empty list, used to represent "false".
 *   - tru: Symbol that evaluates to itself, used for explicit truth in
 *     boolean functions (predicates).
 *   - *debug-trace*: List of functions that are currently being traced by
 *     the debugger.
 */
extern struct Expr* g_nil;
extern struct Expr* g_tru;
extern struct Expr* g_debug_trace_list;

/*----------------------------------------------------------------------------*/

/*
 * Allocate and return an empty 'Env' structure without specifying the parent.
 */
Env* env_new(void);

/*
 * Initialize environment with the default symbols like "nil" and primitives.
 */
void env_init_defaults(Env* env);

/*
 * Clone a linked list of Env structures into new allocated memory.
 */
Env* env_clone(Env* env);

/*
 * Free all elements of an Env structure, and the structure itself.
 */
void env_free(Env* env);

/*----------------------------------------------------------------------------*/

/*
 * Bind the symbol 'sym' to the expression 'val' in environment 'env', with the
 * specified 'flags'.
 *
 * Returns 'ENV_ERR_NONE' (zero) on success, or non-zero on failure. The caller
 * is responsible for checking the returned value, handling errors and
 * optionally printing them with 'env_strerror'.
 */
enum EEnvErr env_bind(Env* env, const char* sym, const struct Expr* val,
                      enum EEnvBindingFlags flags);

/*
 * Bind the symbol 'sym' to the expression 'val' in the top-most parent of
 * environment 'env', with the specified 'flags'.
 */
enum EEnvErr env_bind_global(Env* env, const char* sym, const struct Expr* val,
                             enum EEnvBindingFlags flags);

/*
 * Get a copy of the expression associated to the symbol 'sym' in environment
 * 'env', or in parent environments. The returned copy must be freed by the
 * caller.
 *
 * Returns NULL if the expression is not found.
 */
struct Expr* env_get(const Env* env, const char* sym);

/*
 * Get the flags of the specified symbol in the specified environment. Returns
 * ENV_FLAG_NONE if the symbol is not bound.
 *
 * TODO: This function should return a unique value if the symbol is not bound,
 * not ENV_FLAG_NONE. This is not currently (4d3cc0f) a problem, but might be in
 * the future.
 */
enum EEnvBindingFlags env_get_flags(const Env* env, const char* sym);

/*----------------------------------------------------------------------------*/

/*
 * Print environment in Lisp list format.
 */
void env_print(FILE* fp, const Env* env);

/*----------------------------------------------------------------------------*/

/*
 * Return an immutable string that describes the specified environment error.
 */
static inline const char* env_strerror(enum EEnvErr code) {
    const char* s;
    switch (code) {
        case ENV_ERR_NONE:
            s = "No error.";
            break;
        case ENV_ERR_CONST:
            s = "Cannot overwrite constant variable.";
            break;
    }
    return s;
}

#endif /* ENV_H_ */
