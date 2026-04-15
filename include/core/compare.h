#ifndef _H_CUBEC_CORE_COMPARE_
#define _H_CUBEC_CORE_COMPARE_
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef int (*compare_fn_t)(const void *, const void *, void *);
#ifdef __cplusplus
}
#endif
#endif