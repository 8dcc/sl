
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/util.h"
#include "include/read.h"
#include "include/primitives.h"

static bool is_char_in_str(char c, const char* str) {
    for (; *str != '\0'; str++)
        if (*str == c)
            return true;

    return false;
}

/*----------------------------------------------------------------------------*/

Expr* prim_read(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_UNUSED(e);
    SL_ON_ERR(return NULL);

    char* str = read_expr(stdin);
    SL_EXPECT(str != NULL, "Error reading expression.");

    Expr* ret  = expr_new(EXPR_STRING);
    ret->val.s = str;
    return ret;
}

Expr* prim_write(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 1);

    expr_print(e);
    return env_get(env, "tru");
}

Expr* prim_read_str(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);

    /*
     * (read-str &optional delimiters)
     *
     * The `read-string' primitive reads characters from `stdin' until one of
     * the following is encountered:
     *   - End-of-file (EOF)
     *   - Null character ('\0')
     *   - A character in the string DELIMITERS.
     * The DELIMITERS string defaults to "\n" (a single newline).
     */
    const size_t arg_num = expr_list_len(e);
    SL_EXPECT(arg_num <= 1, "Too many arguments");

    const char* delimiters = "\n";
    if (arg_num == 1) {
        SL_EXPECT_TYPE(e, EXPR_STRING);
        delimiters = e->val.s;
    }

    size_t str_pos = 0;
    size_t str_sz  = 100;
    char* str      = sl_safe_malloc(str_sz);

    for (;;) {
        if (str_pos >= str_sz - 1) {
            str_sz += 100;
            sl_safe_realloc(str, str_sz);
        }

        const char c = getchar();
        if (c == EOF || c == '\0' || is_char_in_str(c, delimiters))
            break;

        str[str_pos++] = c;
    }

    str[str_pos] = '\0';

    Expr* ret  = expr_new(EXPR_STRING);
    ret->val.s = str;
    return ret;
}

Expr* prim_print_str(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_STRING);

    printf("%s", e->val.s);
    return expr_clone(e);
}

Expr* prim_error(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_STRING);

    sl_print_err(false, "error", "%s", e->val.s);

	/* Intentionally return NULL to stop execution */
    return NULL;
}
