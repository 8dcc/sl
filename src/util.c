
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/util.h"

void err_msg(const char* func, const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);

    fprintf(stderr, "%s: ", func);
    vfprintf(stderr, fmt, va);
    fputc('\n', stderr);

    va_end(va);
}

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
