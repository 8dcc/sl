
#ifndef SL_VALGRIND_MEMCHECK_H_
#define SL_VALGRIND_MEMCHECK_H_ 1

#if defined(SL_NO_POOL_VALGRIND)
#define VALGRIND_MAKE_MEM_DEFINED(a, b)  ((void)0)
#define VALGRIND_MAKE_MEM_NOACCESS(a, b) ((void)0)
#else
#include <valgrind/memcheck.h>
#endif

#endif /* SL_VALGRIND_MEMCHECK_H_ */
