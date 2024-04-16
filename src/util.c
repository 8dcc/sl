
#include <stdarg.h>
#include <stdio.h>
#include "include/util.h"

void err_msg(const char* func, const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);

    fprintf(stderr, "%s: ", func);
    vfprintf(stderr, fmt, va);
    fputc('\n', stderr);

    va_end(va);
}
