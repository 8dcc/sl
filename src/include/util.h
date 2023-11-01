#ifndef UTIL_H_
#define UTIL_H_ 1

/* Wrapper for err_msg() */
#define ERR(...) err_msg(__func__, __VA_ARGS__)

/* Print error message to stderr, along with the function name */
void err_msg(const char* func, const char* fmt, ...);

#endif /* UTIL_H_ */
