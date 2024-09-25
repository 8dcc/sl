
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>

#include "include/util.h"

void err_msg(const char* func, const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);

    fprintf(stderr, "%s: ", func);
    vfprintf(stderr, fmt, va);
    fputc('\n', stderr);

    va_end(va);
}

/*----------------------------------------------------------------------------*/

void* sl_safe_malloc(size_t size) {
    void* result = malloc(size);
    SL_ASSERT(result != NULL, "Failed to allocate %zu bytes: %s (%d).", size,
              strerror(errno), errno);
    return result;
}

void* sl_safe_calloc(size_t nmemb, size_t size) {
    void* result = calloc(nmemb, size);
    SL_ASSERT(result != NULL,
              "Failed to allocate %zu elements of %zu bytes each: %s (%d).",
              nmemb, size, strerror(errno), errno);
    return result;
}

char* sl_safe_strdup(const char* s) {
    char* result = strdup(s);
    SL_ASSERT(result != NULL, "Failed to copy string: %s (%d).",
              strerror(errno), errno);
    return result;
}

/*----------------------------------------------------------------------------*/

/* clang-format off */
char escaped2byte(char escaped) {
    switch (escaped) {
        case 'a':  return '\a';
        case 'b':  return '\b';
        case 'e':  return '\e';
        case 'f':  return '\f';
        case 'n':  return '\n';
        case 'r':  return '\r';
        case 't':  return '\n';
        case 'v':  return '\v';
        case '\\': return '\\';
        case '\"': return '\"';
        default:
            ERR("The specified escape sequence (\\%c) is not currently "
                "supported.",
                escaped);
            return escaped;
    }
}

const char* byte2escaped(char byte) {
    switch (byte) {
        case '\a': return "\\a";
        case '\b': return "\\b";
        case '\e': return "\\e";
        case '\f': return "\\f";
        case '\n': return "\\n";
        case '\r': return "\\r";
        case '\t': return "\\n";
        case '\v': return "\\v";
        case '\\': return "\\\\";
        case '\"': return "\\\"";
        default:   return NULL;
    }
}
/* clang-format on */

void print_escaped_str(const char* s) {
    SL_ASSERT(s != NULL, "Got NULL string.");

    putchar('\"');
    for (; *s != '\0'; s++) {
        const char* escape_sequence = byte2escaped(*s);
        if (escape_sequence != NULL)
            printf("%s", escape_sequence);
        else
            printf("%c", *s);
    }
    putchar('\"');
}

/*----------------------------------------------------------------------------*/

bool sl_regex_matches(const char* pat, const char* str, bool ignore_case,
                      size_t* nmatch, regmatch_t** pmatch) {
    /*
     * TODO: Don't use POSIX regex syntax.
     * For example:
     *
     *   "([[:alpha:]]{2,3}) ([[:space:][:digit:]]+)"
     *
     * Instead of:
     *
     *   "([a-z]{2,3}) ([\s\d+])"
     *
     * Even though "[[:CLASS:]]" is supported in a lot of RE flavors.
     *
     * TODO: If the syntax changes, update comments in `prim_string_matches'.
     */
    static regex_t r;

    int cflags = REG_EXTENDED;
    if (ignore_case)
        cflags |= REG_ICASE;

    if (regcomp(&r, pat, cflags) != REG_NOERROR) {
        ERR("Failed to compile pattern \"%s\"", pat);
        return false;
    }

    /*
     * The size of the match array is the number of sub-expressions plus one
     * extra item for the entire match, which will be at index 0.
     */
    *nmatch = r.re_nsub + 1;
    *pmatch = malloc(*nmatch * sizeof(regmatch_t*));

    const int code = regexec(&r, str, *nmatch, *pmatch, 0);
    regfree(&r);

    if (code != REG_NOERROR) {
        free(*pmatch);
        *nmatch = 0;
        *pmatch = NULL;
        return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

size_t int2str(long long x, char** dst) {
    /*
     * A call to `snprintf' with (NULL, 0, ...) as arguments can be used to get
     * the number of bytes to be written. We still have to add one for the
     * null-terminator.
     */
    const int size = snprintf(NULL, 0, "%lld", x);
    if (size < 0) {
        ERR("Failed to get the target string length for integer: %lld", x);
        *dst = NULL;
        return 0;
    }

    *dst = sl_safe_malloc(size + 1);
    snprintf(*dst, size + 1, "%lld", x);
    return size;
}

size_t flt2str(double x, char** dst) {
    const int size = snprintf(NULL, 0, "%f", x);
    if (size < 0) {
        ERR("Failed to get the target string length for float: %f", x);
        *dst = NULL;
        return 0;
    }

    *dst = sl_safe_malloc(size + 1);
    snprintf(*dst, size + 1, "%f", x);
    return size;
}
