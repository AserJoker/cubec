#ifndef _H_CUBEC_ENGINE_STR_TYPE_
#define _H_CUBEC_ENGINE_STR_TYPE_
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ---- Mutable str type singleton ---- */

type_t type_get_str_type(allocator_t allocator);

/* ---- Const str type singleton ---- */

type_t type_get_const_str_type(allocator_t allocator);

/* ---- Value constructor ---- */

value_t create_str_value(vm_t vm, const char *val);

#ifdef __cplusplus
}
#endif
#endif
