
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/util.h"
#include "include/primitives.h"

#define FORMAT_BUFSZ 100

Expr* prim_format(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");
    SL_EXPECT_TYPE(e, EXPR_STRING);

    const char* fmt     = e->val.s;
    const Expr* cur_arg = e->next;

    size_t dst_pos = 0;
    size_t dst_sz  = FORMAT_BUFSZ;
    char* dst      = sl_safe_malloc(FORMAT_BUFSZ);

    while (*fmt != '\0') {
        if (dst_pos >= dst_sz - 1) {
            dst_sz += FORMAT_BUFSZ;
            sl_safe_realloc(dst, dst_sz);
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
        if (cur_arg == NULL) {
            SL_ERR("Not enough arguments for the specified format.");
            free(dst);
            return NULL;
        }

        /*
         * Depending on the format specifier, store the C format string and the
         * expression type of the value.
         */
        const char* c_format     = NULL;
        enum EExprType expr_type = EXPR_ERR;
        switch (*fmt) {
            case 's':
                c_format  = "%s";
                expr_type = EXPR_STRING;
                break;

            case 'd':
                c_format  = "%lld";
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
                SL_ERR("Invalid format specifier: '%c' (0x%02x).", *fmt, *fmt);
                dst[dst_pos++] = *fmt++;
                continue;
        }
        fmt++;

        /*
         * Make sure the current format specifier is valid for the current
         * argument.
         */
        if (expr_type != cur_arg->type) {
            SL_ERR("Format specifier expected argument of type '%s', got '%s'.",
                   exprtype2str(expr_type), exprtype2str(cur_arg->type));
            free(dst);
            return NULL;
        }

        /*
         * Write the actual value to the destination string, using the C format
         * string and the expression type.
         */
        switch (expr_type) {
            case EXPR_STRING: {
                const size_t str_len = strlen(cur_arg->val.s);
                if (dst_pos + str_len >= dst_sz - 1) {
                    dst_sz += str_len + FORMAT_BUFSZ;
                    sl_safe_realloc(dst, dst_sz);
                }

                const int written = snprintf(&dst[dst_pos], str_len + 1,
                                             c_format, cur_arg->val.s);
                dst_pos += written;
            } break;

            case EXPR_NUM_INT: {
                const int str_len = snprintf(NULL, 0, c_format, cur_arg->val.n);
                if (dst_pos + str_len >= dst_sz - 1) {
                    dst_sz += str_len + FORMAT_BUFSZ;
                    sl_safe_realloc(dst, dst_sz);
                }

                const int written = snprintf(&dst[dst_pos], str_len + 1,
                                             c_format, cur_arg->val.n);
                dst_pos += written;
            } break;

            case EXPR_NUM_FLT: {
                const int str_len = snprintf(NULL, 0, c_format, cur_arg->val.f);
                if (dst_pos + str_len >= dst_sz - 1) {
                    dst_sz += str_len + FORMAT_BUFSZ;
                    sl_safe_realloc(dst, dst_sz);
                }

                const int written = snprintf(&dst[dst_pos], str_len + 1,
                                             c_format, cur_arg->val.f);
                dst_pos += written;
            } break;

            default:
                SL_FATAL("Unexpected expression type after processing format "
                         "specifier.");
        }

        /* Move to the next argument, for the next format specifier. */
        cur_arg = cur_arg->next;
    }

done:
    dst[dst_pos] = '\0';

    /*
     * NOTE: We could warn the user if `cur_arg != NULL', since that means he
     * specified to many arguments for this format.
     */

    Expr* ret  = expr_new(EXPR_STRING);
    ret->val.s = dst;
    return ret;
}

Expr* prim_substring(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);

    const size_t arg_num = expr_list_len(e);
    SL_EXPECT(arg_num >= 1 || arg_num <= 3,
              "Expected between 1 and 3 arguments.");

    SL_EXPECT_TYPE(e, EXPR_STRING);
    const long long str_len = (long long)strlen(e->val.s);
    long long start_idx     = 0;
    long long end_idx       = str_len;

    /* Second argument, start index */
    if (arg_num >= 2) {
        SL_EXPECT_TYPE(e->next, EXPR_NUM_INT);
        start_idx = e->next->val.n;
        if (start_idx < 0)
            start_idx += str_len;
    }

    /* Third argument, end index */
    if (arg_num >= 3) {
        SL_EXPECT_TYPE(e->next->next, EXPR_NUM_INT);
        end_idx = e->next->next->val.n;
        if (end_idx < 0)
            end_idx += str_len;
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
    ret->val.s = sl_safe_malloc(end_idx - start_idx + 1);

    long long dst_i, src_i;
    for (dst_i = 0, src_i = start_idx; src_i < end_idx; dst_i++, src_i++)
        ret->val.s[dst_i] = e->val.s[src_i];
    ret->val.s[dst_i] = '\0';

    return ret;
}

Expr* prim_string_matches(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);

    const size_t arg_num = expr_list_len(e);
    SL_EXPECT(arg_num >= 2 || arg_num <= 3, "Expected 2 or 3 arguments.");

    SL_EXPECT_TYPE(e, EXPR_STRING);
    const char* pattern = e->val.s;

    SL_EXPECT_TYPE(e->next, EXPR_STRING);
    const char* string = e->next->val.s;

    const bool ignore_case = (arg_num >= 3 && !expr_is_nil(e->next->next));

    /*
     * Argument syntax: (regexp string &optional ignore-case)
     *
     * The `string-matches' function returns a list of matches. The first match
     * corresponds to the entire regular expression, and the rest correspond to
     * each parenthesized sub-expression.  Only the matches are included in the
     * returned list, so `nil' means that no match was found for the entire
     * expression.
     *
     * Each item in the returned list is a list of two integers corresponding to
     * the start and end indexes of that match inside `string'.
     *
     * It uses Extended Regular Expression (ERE) syntax. See:
     *   https://www.gnu.org/software/sed/manual/html_node/ERE-syntax.html
     *   https://www.gnu.org/software/sed/manual/html_node/BRE-vs-ERE.html
     *   https://www.gnu.org/software/sed/manual/html_node/Character-Classes-and-Bracket-Expressions.html
     */
    Expr* ret = expr_new(EXPR_PARENT);

    size_t nmatch;
    regmatch_t* pmatch;
    if (!sl_regex_matches(pattern, string, ignore_case, &nmatch, &pmatch)) {
        ret->val.children = NULL;
        return ret;
    }

    Expr dummy_copy;
    dummy_copy.next = NULL;
    Expr* cur_copy  = &dummy_copy;

    for (size_t i = 0; i < nmatch; i++) {
        if (pmatch[i].rm_so == -1 || pmatch[i].rm_eo == -1)
            break;

        /* (cons start-offset end-offset) */
        Expr* match_pair                      = expr_new(EXPR_PARENT);
        match_pair->val.children              = expr_new(EXPR_NUM_INT);
        match_pair->val.children->val.n       = pmatch[i].rm_so;
        match_pair->val.children->next        = expr_new(EXPR_NUM_INT);
        match_pair->val.children->next->val.n = pmatch[i].rm_eo;

        cur_copy->next = match_pair;
        cur_copy       = cur_copy->next;
    }

    ret->val.children = dummy_copy.next;

    free(pmatch);
    return ret;
}
