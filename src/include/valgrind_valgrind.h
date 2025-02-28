
#ifndef SL_VALGRIND_VALGRIND_H_
#define SL_VALGRIND_VALGRIND_H_ 1

#if defined(SL_NO_POOL_VALGRIND)
#define VALGRIND_CREATE_MEMPOOL(a, b, c) ((void)0)
#define VALGRIND_DESTROY_MEMPOOL(a)      ((void)0)
#define VALGRIND_MEMPOOL_ALLOC(a, b, c)  ((void)0)
#define VALGRIND_MEMPOOL_FREE(a, b)      ((void)0)
#else
#include <valgrind/valgrind.h>
#endif

#endif /* SL_VALGRIND_VALGRIND_H_ */
