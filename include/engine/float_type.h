#ifndef _H_CUBEC_ENGINE_FLOAT_TYPE_
#define _H_CUBEC_ENGINE_FLOAT_TYPE_
#include "engine/type.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* ---- Mutable float type singletons ---- */

type_t type_get_f16_type(allocator_t allocator);
type_t type_get_f32_type(allocator_t allocator);
type_t type_get_f64_type(allocator_t allocator);

/* ---- Const float type singletons ---- */

type_t type_get_const_f16_type(allocator_t allocator);
type_t type_get_const_f32_type(allocator_t allocator);
type_t type_get_const_f64_type(allocator_t allocator);

/* ---- Value constructors ---- */

value_t create_f16_value(vm_t vm, uint16_t bits);
value_t create_f32_value(vm_t vm, float val);
value_t create_f64_value(vm_t vm, double val);

#ifdef __cplusplus
}
#endif
#endif
