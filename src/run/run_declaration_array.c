#include "run/run.h"
#include "engine/vm.h"
#include "engine/exception_type.h"
#include "engine/value.h"
#include "engine/type.h"
#include "cubec/declaration_array.h"
#include <string.h>

value_t run_declaration_array(vm_t vm, node_t node, bool shadow) {
  (void)shadow; /* type declarations have no shadow scenario */
  cubec_declaration_array_t arr = (cubec_declaration_array_t)node;

  /* evaluate size expression */
  value_t size_val = run_expression(vm, arr->size, false);
  if (value_is_abnormal(size_val)) return size_val;

  /* size must be an integer value */
  type_kind_t size_kind = type_get_kind(value_get_type(size_val));
  if (size_kind != TYPE_KIND_U8 && size_kind != TYPE_KIND_U16 &&
      size_kind != TYPE_KIND_U32 && size_kind != TYPE_KIND_U64 &&
      size_kind != TYPE_KIND_I8 && size_kind != TYPE_KIND_I16 &&
      size_kind != TYPE_KIND_I32 && size_kind != TYPE_KIND_I64)
    return create_exception_value(vm, "array size must be an integer, got '%s'",
                                  type_get_name(value_get_type(size_val)));
  uint64_t count = 0;
  memcpy(&count, value_get_data(size_val), (size_t)type_get_size(value_get_type(size_val)));

  /* evaluate element type expression */
  value_t elem_type_val = run_expression(vm, arr->type, false);
  if (value_is_abnormal(elem_type_val)) return elem_type_val;

  /* element type expression must produce a type value */
  if (type_get_kind(value_get_type(elem_type_val)) != TYPE_KIND_TYPE)
    return create_exception_value(vm, "array element type expression must produce a type, got '%s'",
                                  type_get_name(value_get_type(elem_type_val)));
  type_t elem_type = (type_t)value_get_data(elem_type_val);

  return vm_create_array_type_value(vm, elem_type, count, true);
}
