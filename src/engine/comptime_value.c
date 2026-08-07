#include "engine/comptime_value.h"
#include "engine/integer_type.h"
#include "engine/float_type.h"
#include "engine/bool_type.h"
#include "engine/char_type.h"
#include "engine/str_type.h"
#include "engine/nil_type.h"
#include "engine/struct_instance.h"
#include "engine/union_instance.h"
#include "engine/cunion_instance.h"
#include "engine/tuple_type.h"
#include "engine/array_type.h"
#include "engine/slice_type.h"
#include "engine/callable_type.h"
#include "engine/function_instance.h"

/* --------------------------------------------------------------------------
 *  Dispose — dispatch to per-type implementation
 * -------------------------------------------------------------------------- */

void comptime_value_dispose(comptime_value_t val) {
  if (!val) return;
  switch (val->kind) {
  case COMPTIME_VALUE_INT:
    if (val->type && val->type->type_kind == TYPE_CHAR)
      char_type_dispose_value(val);
    else
      integer_type_dispose_value(val);
    break;
  case COMPTIME_VALUE_FLOAT:
    float_type_dispose_value(val);
    break;
  case COMPTIME_VALUE_BOOL:
    bool_type_dispose_value(val);
    break;
  case COMPTIME_VALUE_STRING:
    str_type_dispose_value(val);
    break;
  case COMPTIME_VALUE_NIL:
    nil_type_dispose_value(val);
    break;
  case COMPTIME_VALUE_STRUCT:
    struct_instance_dispose_value(val);
    break;
  case COMPTIME_VALUE_UNION:
    union_instance_dispose_value(val);
    break;
  case COMPTIME_VALUE_CUNION:
    cunion_instance_dispose_value(val);
    break;
  case COMPTIME_VALUE_TUPLE:
    tuple_type_dispose_value(val);
    break;
  case COMPTIME_VALUE_ARRAY:
    array_type_dispose_value(val);
    break;
  case COMPTIME_VALUE_SLICE:
    slice_type_dispose_value(val);
    break;
  case COMPTIME_VALUE_CALLABLE:
    callable_type_dispose_value(val);
    break;
  case COMPTIME_VALUE_FUNCTION:
    function_instance_dispose_value(val);
    break;
  }
}

/* --------------------------------------------------------------------------
 *  Clone — dispatch to per-type implementation
 * -------------------------------------------------------------------------- */

comptime_value_t comptime_value_clone(allocator_t allocator, comptime_value_t val) {
  if (!val) return NULL;
  switch (val->kind) {
  case COMPTIME_VALUE_INT:
    if (val->type && val->type->type_kind == TYPE_CHAR)
      return char_type_clone_value(allocator, val);
    return integer_type_clone_value(allocator, val);
  case COMPTIME_VALUE_FLOAT:
    return float_type_clone_value(allocator, val);
  case COMPTIME_VALUE_BOOL:
    return bool_type_clone_value(allocator, val);
  case COMPTIME_VALUE_STRING:
    return str_type_clone_value(allocator, val);
  case COMPTIME_VALUE_NIL:
    return nil_type_clone_value(allocator, val);
  case COMPTIME_VALUE_STRUCT:
    return struct_instance_clone_value(allocator, val);
  case COMPTIME_VALUE_UNION:
    return union_instance_clone_value(allocator, val);
  case COMPTIME_VALUE_CUNION:
    return cunion_instance_clone_value(allocator, val);
  case COMPTIME_VALUE_TUPLE:
    return tuple_type_clone_value(allocator, val);
  case COMPTIME_VALUE_ARRAY:
    return array_type_clone_value(allocator, val);
  case COMPTIME_VALUE_SLICE:
    return slice_type_clone_value(allocator, val);
  case COMPTIME_VALUE_CALLABLE:
    return callable_type_clone_value(allocator, val);
  case COMPTIME_VALUE_FUNCTION:
    return function_instance_clone_value(allocator, val);
  }
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Hash — dispatch to per-type implementation
 * -------------------------------------------------------------------------- */

uint64_t comptime_value_hash(comptime_value_t val) {
  if (!val) return 0;
  switch (val->kind) {
  case COMPTIME_VALUE_INT:
    if (val->type && val->type->type_kind == TYPE_CHAR)
      return char_type_hash_value(val);
    return integer_type_hash_value(val);
  case COMPTIME_VALUE_FLOAT:
    return float_type_hash_value(val);
  case COMPTIME_VALUE_BOOL:
    return bool_type_hash_value(val);
  case COMPTIME_VALUE_STRING:
    return str_type_hash_value(val);
  case COMPTIME_VALUE_NIL:
    return nil_type_hash_value(val);
  case COMPTIME_VALUE_STRUCT:
    return struct_instance_hash_value(val);
  case COMPTIME_VALUE_UNION:
    return union_instance_hash_value(val);
  case COMPTIME_VALUE_CUNION:
    return cunion_instance_hash_value(val);
  case COMPTIME_VALUE_TUPLE:
    return tuple_type_hash_value(val);
  case COMPTIME_VALUE_ARRAY:
    return array_type_hash_value(val);
  case COMPTIME_VALUE_SLICE:
    return slice_type_hash_value(val);
  case COMPTIME_VALUE_CALLABLE:
    return callable_type_hash_value(val);
  case COMPTIME_VALUE_FUNCTION:
    return function_instance_hash_value(val);
  }
  return 0;
}
