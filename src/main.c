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

#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> /* isatty() */
#include <time.h>   /* time() */

#include "include/env.h"
#include "include/expr.h"
#include "include/expr_pool.h"
#include "include/util.h"
#include "include/garbage_collector.h"
#include "include/read.h"
#include "include/lexer.h"
#include "include/parser.h"
#include "include/eval.h"

#define STDLIB_PATH "/usr/local/lib/sl/stdlib.lisp"

static FILE* get_input_file(int argc, char** argv) {
    FILE* result = stdin;

    if (argc > 1) {
        const char* filename = argv[1];
        result               = fopen(filename, "r");
        if (result == NULL) {
            fprintf(stderr,
                    "Error opening '%s': %s.\n",
                    filename,
                    strerror(errno));
            exit(1);
        }
    }

    return result;
}

static void repl_until_eof(Env* env, FILE* file, bool print_prompt,
                           bool print_evaluated) {
    for (;;) {
        if (print_prompt)
            printf("\nsl> ");

        /*
         * Allocate string and read an expression. If 'read_expr' returned NULL,
         * it encountered EOF.
         */
        char* input = read_expr(file);
        if (input == NULL) {
            if (print_prompt)
                putchar('\n');
            break;
        }

        /* Tokenize input. We don't need to check for NULL. */
        Token* tokens = tokenize(input);

        /* We are done with the string from 'read_expr', free it */
        free(input);

        /* Get expression (AST) from token array */
        Expr* expr = parse(tokens);

        /* We are done with the token array, free it */
        tokens_free(tokens);

        if (expr == NULL)
            continue;

        /* Evaluate expression recursivelly */
        Expr* evaluated = eval(env, expr);
        if (evaluated == NULL)
            continue;

        if (print_evaluated)
            expr_println(EXPR_ERR_P(evaluated) ? stderr : stdout, evaluated);

        /*
         * Collect all garbage that is not in the current environment.
         *
         * TODO: Marking globals is temporary, until we save references directly
         * in the environment.
         */
        gc_unmark_all();
        gc_mark_expr(g_nil);
        gc_mark_expr(g_tru);
        gc_mark_expr(g_debug_trace_list);
        gc_mark_env(env);
        gc_collect();
    }
}

int main(int argc, char** argv) {
    FILE* file_input        = get_input_file(argc, argv);
    const bool print_prompt = (file_input == stdin && isatty(0));

    /* Allocate the expression pool. It will be expanded when needed. */
    if (!pool_init(BASE_POOL_SZ))
        SL_FATAL("Failed to initialize pool of %zu expressions.\n",
                 BASE_POOL_SZ);

    /* Initialize global environment with symbols like "nil" */
    Env* global_env = env_new();
    SL_ASSERT(global_env != NULL);
    env_init_defaults(global_env);

    /* Set unique random seed, can be overwritten with 'prim_set_random_seed' */
    srand(time(NULL));

#ifndef SL_NO_STDLIB
    /* Try to silently load the standard library from the current directory */
    FILE* file_stdlib = fopen(STDLIB_PATH, "r");
    if (file_stdlib != NULL) {
        repl_until_eof(global_env, file_stdlib, false, false);
        fclose(file_stdlib);
        fprintf(stderr, "Standard library loaded.\n");
    }
#endif

    if (print_prompt)
        fprintf(stderr, "Welcome to the Simple Lisp REPL.\n");

    repl_until_eof(global_env, file_input, print_prompt, true);

    env_free(global_env);
    pool_close();
    fclose(file_input);
    return 0;
}
