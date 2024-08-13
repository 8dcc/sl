
#include <stdarg.h>
#include <stdio.h>
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
    if (result == NULL) {
        ERR("Failed to allocate %zu bytes.", size);
        abort();
    }
    return result;
}

void* sl_safe_calloc(size_t nmemb, size_t size) {
    void* result = calloc(nmemb, size);
    if (result == NULL) {
        ERR("Failed to allocate %zu elements of %zu bytes each.", nmemb, size);
        abort();
    }
    return result;
}
