
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* isatty() */

#include "include/env.h"
#include "include/expr.h"
#include "include/util.h"
#include "include/read.h"
#include "include/lexer.h"
#include "include/parser.h"
#include "include/eval.h"

int main(void) {
    const bool print_prompt = isatty(0);

    /* Initialize global environment with symbols like "nil" */
    Env* global_env = env_new();
    env_init_defaults(global_env);

    bool got_eof = false;
    while (!got_eof) {
        if (print_prompt)
            printf("sl> ");

        /* Allocate string and read an expression. If `read' returned NULL, it
         * encountered EOF. */
        char* input = read_expr(stdin);
        if (input == NULL)
            break;

        /* Tokenize input. We don't need to check for NULL. */
        Token* tokens = tokenize(input);

        /* We are done with the string from `read', free it */
        free(input);

        /* Get expression (AST) from token array */
        Expr* expr = parse(tokens);

        /* We are done with the token array, free it */
        tokens_free(tokens);

        if (expr == NULL)
            continue;

        /* Evaluate expression recursivelly */
        Expr* evaluated = eval(global_env, expr);

        /* We are done with the original expression */
        expr_free(expr);

        if (evaluated == NULL)
            continue;

        expr_println(evaluated);

        /* Free the evaluated expression */
        expr_free(evaluated);

        if (print_prompt)
            putchar('\n');
    }

    env_free(global_env);

    return 0;
}
