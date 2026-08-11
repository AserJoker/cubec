#ifndef _H_CUBEC_ENGINE_INTEGER_TYPE_
#define _H_CUBEC_ENGINE_INTEGER_TYPE_
#include "engine/type.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* ---- Mutable signed integer type singletons ---- */

type_t type_get_i8_type(allocator_t allocator);
type_t type_get_i16_type(allocator_t allocator);
type_t type_get_i32_type(allocator_t allocator);
type_t type_get_i64_type(allocator_t allocator);

/* ---- Const signed integer type singletons ---- */

type_t type_get_const_i8_type(allocator_t allocator);
type_t type_get_const_i16_type(allocator_t allocator);
type_t type_get_const_i32_type(allocator_t allocator);
type_t type_get_const_i64_type(allocator_t allocator);

/* ---- Mutable unsigned integer type singletons ---- */

type_t type_get_u8_type(allocator_t allocator);
type_t type_get_u16_type(allocator_t allocator);
type_t type_get_u32_type(allocator_t allocator);
type_t type_get_u64_type(allocator_t allocator);

/* ---- Const unsigned integer type singletons ---- */

type_t type_get_const_u8_type(allocator_t allocator);
type_t type_get_const_u16_type(allocator_t allocator);
type_t type_get_const_u32_type(allocator_t allocator);
type_t type_get_const_u64_type(allocator_t allocator);

/* ---- Signed value constructors ---- */

value_t create_i8_value(vm_t vm, int8_t val);
value_t create_i16_value(vm_t vm, int16_t val);
value_t create_i32_value(vm_t vm, int32_t val);
value_t create_i64_value(vm_t vm, int64_t val);

/* ---- Unsigned value constructors ---- */

value_t create_u8_value(vm_t vm, uint8_t val);
value_t create_u16_value(vm_t vm, uint16_t val);
value_t create_u32_value(vm_t vm, uint32_t val);
value_t create_u64_value(vm_t vm, uint64_t val);

#ifdef __cplusplus
}
#endif
#endif
