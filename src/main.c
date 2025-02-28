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
#include "include/error.h"
#include "include/debug.h"
#include "include/cmdargs.h"
#include "include/read.h"
#include "include/lexer.h"
#include "include/parser.h"
#include "include/eval.h"

#define STDLIB_PATH "/usr/local/lib/sl/stdlib.lisp"

static void repl_until_eof(Env* env, FILE* file, bool print_evaluated,
                           bool print_prompt) {
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
         */
        gc_unmark_all();
        gc_mark_env_contents(env);
        gc_collect();
    }
}

int main(int argc, char** argv) {
    CmdArgs cmd_args           = cmdargs_parse(argc, argv);
    const bool interactive_run = (cmd_args.input_files_sz == 0 && isatty(0));

    /*
     * Allocate the initial expression pool. It will be expanded when needed.
     */
    if (!pool_init(POOL_BASE_SZ))
        SL_FATAL("Failed to initialize the expression pool.");

    /*
     * Initialize the callstack.
     */
    if (!debug_callstack_init())
        SL_FATAL("Failed to initialize the Lisp callstack.");

    /*
     * Initialize global environment with the primitives and with symbols like
     * 'nil'.
     */
    Env* global_env = env_new();
    SL_ASSERT(global_env != NULL);
    env_init_defaults(global_env);

    /*
     * Set unique random seed, can be overwritten with the `set-random-seed'
     * Lisp primitive.
     */
    srand(time(NULL));

    /*
     * Try to silently load the standard library from the known path.
     */
    if (cmd_args.load_sys_stdlib) {
        FILE* file_stdlib = fopen(STDLIB_PATH, "r");
        if (file_stdlib == NULL) {
            fprintf(stderr,
                    "Warning: Couldn't open standard library from '%s'.\n",
                    STDLIB_PATH);
        } else {
            repl_until_eof(global_env, file_stdlib, false, false);
            fclose(file_stdlib);
            fprintf(stderr, "Standard library loaded.\n");
        }
    }

    if (interactive_run) {
        /*
         * The user didn't specify any input files in the command-line, start an
         * interactive REPL from 'stdin'.
         */
        fprintf(stderr, "Welcome to the Simple Lisp REPL.\n");
        repl_until_eof(global_env, stdin, true, true);
    } else {
        /*
         * Non-interactive run, just parse each input file sequencially.
         */
        for (size_t i = 0; i < cmd_args.input_files_sz; i++)
            repl_until_eof(global_env,
                           cmd_args.input_files[i].fd,
                           !cmd_args.input_files[i].silent_eval,
                           false);
    }

    env_free(global_env);
    debug_callstack_free();
    pool_close();
    cmdargs_close_files(&cmd_args);
    return 0;
}
