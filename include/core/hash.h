#ifndef _H_CUBEC_CORE_HASH_
#define _H_CUBEC_CORE_HASH_
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef int64_t (*cubec_hash_fn_t)(void *data, void *arg);
#ifdef __cplusplus
}
#endif
#endif