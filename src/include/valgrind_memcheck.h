
#ifndef SL_VALGRIND_MEMCHECK_H_
#define SL_VALGRIND_MEMCHECK_H_ 1

#ifdef SL_NO_POOL_VALGRIND
#define VALGRIND_MAKE_MEM_DEFINED(a, b)  ((void)0)
#define VALGRIND_MAKE_MEM_NOACCESS(a, b) ((void)0)
#else /* not SL_NO_POOL_VALGRIND */
#include <valgrind/memcheck.h>
#endif /* not SL_NO_POOL_VALGRIND */

#endif /* SL_VALGRIND_MEMCHECK_H_ */
