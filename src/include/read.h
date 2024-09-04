
#ifndef READ_H_
#define READ_H_ 1

#include <stdbool.h>
#include <stdio.h> /* FILE */

/* Allocate string and read a single Lisp expression. Returns NULL if it
 * encountered EOF on the last call. */
char* read_expr(FILE* fp);

#endif /* READ_H_ */
