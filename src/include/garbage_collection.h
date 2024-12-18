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

#include "include/env.h"

/*
 * Initialize the garbage collection system.
 */
void gc_init(void);

/*
 * Register a pointer that could be garbage-collected.
 */
void gc_register(void* ptr);

/*
 * Collect (free) the garbage that is not in the specified environment.
 *
 * In order for garbage to be collected, it must have been previously registered
 * using `gc_register'.
 *
 * FIXME: This function assumes that `env' is the only environment in use. This
 * is fine when passing the global environment (as long as it's not duringe.
 * procedure call), but we shouldn't rely on this. Specifically, this might be a
 * problem when adding closures in the future.
 */
void gc_collect_env(Env* env);

#endif /* GARBAGE_COLLECTION_H_ */
