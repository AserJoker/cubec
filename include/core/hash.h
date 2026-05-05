#ifndef _H_CORE_HASH_
#define _H_CORE_HASH_
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef int64_t (*hash_fn_t)(const void *data, void *arg);
#ifdef __cplusplus
}
#endif
#endif