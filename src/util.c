
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

void print_escaped_str(const char* s) {
    putchar('\"');
    for (; *s != '\0'; s++) {
        switch (*s) {
            case '\a':
                printf("\\a");
                break;
            case '\b':
                printf("\\b");
                break;
            case '\e':
                printf("\\e");
                break;
            case '\f':
                printf("\\f");
                break;
            case '\n':
                printf("\\n");
                break;
            case '\r':
                printf("\\r");
                break;
            case '\t':
                printf("\\n");
                break;
            case '\v':
                printf("\\v");
                break;
            case '\\':
                printf("\\\\");
                break;
            case '\"':
                printf("\\\"");
                break;
            default:
                putchar(*s);
                break;
        }
    }
    putchar('\"');
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
