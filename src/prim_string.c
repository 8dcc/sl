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

#define _POSIX_C_SOURCE 200809L /* open_memstream() */

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/util.h"
#include "include/memory.h"
#include "include/primitives.h"

Expr* prim_write_to_str(Env* env, Expr* args) {
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(args, 1);
    const Expr* arg = CAR(args);

    char* str;
    size_t sz;
    FILE* fp = open_memstream(&str, &sz);

    const bool success = expr_write(fp, arg);
    fclose(fp);

    if (!success) {
        free(str);
        return err("Couldn't write expression of type '%s'.",
                   exprtype2str(arg->type));
    }

    Expr* ret  = expr_new(EXPR_STRING);
    ret->val.s = str;
    return ret;
}

/*
 * TODO: Add `read-from-string' function, similar to `read', but from a string.
 */

/*----------------------------------------------------------------------------*/

#define FORMAT_BUFSZ 100

Expr* prim_format(Env* env, Expr* args) {
    SL_UNUSED(env);
    SL_EXPECT(!expr_is_nil(args), "Expected at least a format argument.");

    SL_EXPECT_TYPE(CAR(args), EXPR_STRING);
    const char* fmt = CAR(args)->val.s;
    args            = CDR(args);

    size_t dst_pos = 0;
    size_t dst_sz  = FORMAT_BUFSZ;
    char* dst      = mem_alloc(FORMAT_BUFSZ);

    while (*fmt != '\0') {
        if (dst_pos >= dst_sz - 1) {
            dst_sz += FORMAT_BUFSZ;
            mem_realloc(dst, dst_sz);
        }

        /*
         * Check if the current character is '%', used to escape format
         * specifiers. If it isn't, just insert the character literally. If it
         * is, skip it and proceed with the conversion.
         */
        if (*fmt != '%') {
            dst[dst_pos++] = *fmt++;
            continue;
        }
        fmt++;

        /* Make sure the user supplied enough arguments. */
        if (expr_is_nil(args)) {
            free(dst);
            return err("Not enough arguments for the specified format.");
        }

        /*
         * Depending on the format specifier, store the C format string and the
         * expression type of the value.
         */
        const char* c_format     = NULL;
        enum EExprType expr_type = EXPR_UNKNOWN;
        switch (*fmt) {
            case 's':
                c_format  = "%s";
                expr_type = EXPR_STRING;
                break;

            case 'd':
                c_format  = "%lld";
                expr_type = EXPR_NUM_INT;
                break;

            case 'u':
                c_format  = "%llu";
                expr_type = EXPR_NUM_INT;
                break;

            case 'x':
                c_format  = "%#llx";
                expr_type = EXPR_NUM_INT;
                break;

            case 'f':
                c_format  = "%f";
                expr_type = EXPR_NUM_FLT;
                break;

            case '%':
                dst[dst_pos++] = *fmt++;
                continue;

            case '\0':
                goto done;

            default:
                /* Just print a warning, but don't stop */
                SL_ERR("Invalid format specifier: '%c' (0x%02x).", *fmt, *fmt);
                dst[dst_pos++] = *fmt++;
                continue;
        }
        fmt++;

        /*
         * Make sure the current format specifier is valid for the current
         * argument.
         */
        const Expr* arg = CAR(args);
        if (expr_type != arg->type) {
            free(dst);
            return err("Format specifier expected argument of type '%s', got "
                       "'%s'.",
                       exprtype2str(expr_type),
                       exprtype2str(arg->type));
        }

        /*
         * Write the actual value to the destination string, using the C format
         * string and the expression type.
         */
        switch (expr_type) {
            case EXPR_STRING:
                sl_concat_format(&dst, &dst_sz, &dst_pos, c_format, arg->val.s);
                break;

            case EXPR_NUM_INT:
                sl_concat_format(&dst, &dst_sz, &dst_pos, c_format, arg->val.n);
                break;

            case EXPR_NUM_FLT:
                sl_concat_format(&dst, &dst_sz, &dst_pos, c_format, arg->val.f);
                break;

            default:
                SL_FATAL("Unexpected expression type after processing format "
                         "specifier.");
        }

        /* Move to the next argument, for the next format specifier. */
        args = CDR(args);
    }

done:
    dst[dst_pos] = '\0';

    /*
     * NOTE: We could warn the user if 'args != NULL', since that means he
     * specified to many arguments for this format.
     */

    Expr* ret  = expr_new(EXPR_STRING);
    ret->val.s = dst;
    return ret;
}

/*----------------------------------------------------------------------------*/

Expr* prim_substring(Env* env, Expr* args) {
    SL_UNUSED(env);

    const size_t arg_num = expr_list_len(args);
    SL_EXPECT(arg_num >= 1 || arg_num <= 3,
              "Expected between 1 and 3 arguments.");

    /* First argument, string */
    const Expr* str_expr = expr_list_nth(args, 1);
    SL_EXPECT_TYPE(str_expr, EXPR_STRING);
    const LispInt str_len = (LispInt)strlen(str_expr->val.s);

    /* Second argument, start index */
    LispInt start_idx = 0;
    if (arg_num >= 2) {
        const Expr* start_idx_expr = expr_list_nth(args, 2);
        if (!expr_is_nil(start_idx_expr)) {
            SL_EXPECT_TYPE(start_idx_expr, EXPR_NUM_INT);
            start_idx = start_idx_expr->val.n;
            if (start_idx < 0)
                start_idx += str_len;
        }
    }

    /* Third argument, end index */
    LispInt end_idx = str_len;
    if (arg_num >= 3) {
        const Expr* end_idx_expr = expr_list_nth(args, 3);
        if (!expr_is_nil(end_idx_expr)) {
            SL_EXPECT_TYPE(end_idx_expr, EXPR_NUM_INT);
            end_idx = end_idx_expr->val.n;
            if (end_idx < 0)
                end_idx += str_len;
        }
    }

    /*
     * The substring includes the character at the start index but excludes the
     * character at the end index. The first character is at index zero.
     * Negative numbers can be used to count from the end of the string, so the
     * last character is at index -1.
     *
     * If the start or end indexes are not within bounds, they are clamped.
     */
    end_idx   = CLAMP(end_idx, 0, str_len);
    start_idx = CLAMP(start_idx, 0, end_idx);

    Expr* ret  = expr_new(EXPR_STRING);
    ret->val.s = mem_alloc(end_idx - start_idx + 1);

    LispInt dst_i, src_i;
    for (dst_i = 0, src_i = start_idx; src_i < end_idx; dst_i++, src_i++)
        ret->val.s[dst_i] = str_expr->val.s[src_i];
    ret->val.s[dst_i] = '\0';

    return ret;
}

/*----------------------------------------------------------------------------*/

Expr* prim_re_match_groups(Env* env, Expr* args) {
    SL_UNUSED(env);

    const size_t arg_num = expr_list_len(args);
    SL_EXPECT(arg_num == 2 || arg_num == 3, "Expected 2 or 3 arguments.");

    /*
     * Argument syntax: (regexp string &optional ignore-case)
     *
     * The `re-match-groups' function returns a list of matches. Each item in
     * the returned list is a pair with two integers corresponding to the start
     * and end indexes of that match inside 'string'. Therefore, the structure
     * of the returned list is:
     *
     *   ((START . END)
     *    (START . END)
     *    ...
     *    (START . END))
     *
     * The first match corresponds to the entire regular expression, and the
     * rest correspond to each parenthesized sub-expression. Only the matches
     * are included in the returned list, so `nil' means that no match was found
     * for the entire expression.
     *
     * It uses Extended Regular Expression (ERE) syntax. See:
     *   https://www.gnu.org/software/sed/manual/html_node/ERE-syntax.html
     *   https://www.gnu.org/software/sed/manual/html_node/BRE-vs-ERE.html
     *   https://www.gnu.org/software/sed/manual/html_node/Character-Classes-and-Bracket-Expressions.html
     */
    SL_EXPECT_TYPE(CAR(args), EXPR_STRING);
    const char* pattern = CAR(args)->val.s;

    SL_EXPECT_TYPE(CADR(args), EXPR_STRING);
    const char* string = CADR(args)->val.s;

    const bool ignore_case =
      (arg_num >= 3 && !expr_is_nil(expr_list_nth(args, 3)));

    size_t nmatch;
    regmatch_t* pmatch;
    if (!sl_regex_match_groups(pattern, string, ignore_case, &nmatch, &pmatch))
        return expr_clone(g_nil);

    Expr dummy_copy;
    dummy_copy.val.pair.cdr = g_nil;
    Expr* cur_copy          = &dummy_copy;

    for (size_t i = 0; i < nmatch; i++) {
        if (pmatch[i].rm_so == -1 || pmatch[i].rm_eo == -1)
            break;

        /* (cons start-offset end-offset) */
        Expr* match_pair       = expr_new(EXPR_PAIR);
        CAR(match_pair)        = expr_new(EXPR_NUM_INT);
        CAR(match_pair)->val.n = pmatch[i].rm_so;
        CDR(match_pair)        = expr_new(EXPR_NUM_INT);
        CDR(match_pair)->val.n = pmatch[i].rm_eo;

        CDR(cur_copy) = expr_new(EXPR_PAIR);
        cur_copy      = CDR(cur_copy);
        CAR(cur_copy) = match_pair;
        CDR(cur_copy) = g_nil;
    }

    free(pmatch);
    return dummy_copy.val.pair.cdr;
}
