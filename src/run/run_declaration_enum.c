#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/value.h"
#include "engine/type.h"
#include "engine/enum_type.h"
#include "engine/integer_type.h"
#include "cubec/declaration_enum.h"
#include "cubec/enum_item.h"
#include "cubec/literal_identifier.h"
#include "core/string.h"
#include "core/vec.h"
#include <string.h>

/* ---- helper: wrap a typed integer value as a type value ---- */

static value_t _type_to_value(vm_t vm, type_t t) {
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
  return vm_create_value_ref(vm, type_type, (const void *)t, NULL);
}

/* ---- main entry ---- */

value_t run_declaration_enum(vm_t vm, node_t node, bool shadow) {
  (void)shadow;
  cubec_declaration_enum_t decl = (cubec_declaration_enum_t)node;
  size_t item_count = vec_get_size(decl->items);

  /* resolve the underlying type — default to i32 if no item specifies one */
  type_t resolved = NULL;
  for (size_t i = 0; i < item_count; i++) {
    cubec_enum_item_t item = (cubec_enum_item_t)vec_get(decl->items, i);
    if (!item->type) continue;

    value_t type_val = run_expression(vm, item->type, false);
    if (value_is_abnormal(type_val))
      return type_val;

    if (type_get_kind(value_get_type(type_val)) != TYPE_KIND_TYPE)
      return create_exception_value(vm,
          "enum item type expression must produce a type, got '%s'",
          type_get_name(value_get_type(type_val)));

    type_t t = (type_t)value_get_data(type_val);
    if (!resolved) {
      resolved = t;
    } else if (type_get_kind(resolved) != type_get_kind(t) ||
               type_get_size(resolved) != type_get_size(t)) {
      return create_exception_value(vm,
          "anonymous enum has inconsistent item types");
    }
  }

  if (!resolved) {
    value_t i32_val = vm_get_i32_type(vm);
    resolved = (type_t)value_get_data(i32_val);
  }

  /* create the enum type value (anonymous — name is NULL) */
  value_t underlying_type_val = _type_to_value(vm, resolved);
  value_t type_val = vm_create_enum_type_value(vm, NULL, underlying_type_val,
                                                false, vm_get_current_module_id(vm));
  enum_type_t et = (enum_type_t)value_get_data(type_val);
  uint64_t underlying_size = type_get_size(resolved);

  int64_t auto_val = 0;

  for (size_t i = 0; i < item_count; i++) {
    cubec_enum_item_t item = (cubec_enum_item_t)vec_get(decl->items, i);
    const char *item_name = string_get(
        ((cubec_literal_identifier_t)item->name)->value);

    if (item->value) {
      value_t val = run_expression(vm, item->value, false);
      if (value_is_abnormal(val))
        return val;

      value_t cast_val = value_safe_cast(vm, val, resolved);
      if (value_is_abnormal(cast_val))
        return create_exception_value(vm,
            "anonymous enum item '%s' value cannot be converted to underlying type '%s'",
            item_name, type_get_name(resolved));

      value_t r = enum_type_add_item(vm, et, item_name, value_get_data(cast_val));
      if (value_is_abnormal(r))
        return r;

      if (underlying_size > 0)
        memcpy(&auto_val, value_get_data(cast_val), (size_t)underlying_size);
      auto_val++;
    } else {
      void *buf = NULL;
      if (underlying_size > 0) {
        allocator_t alloc = vm_get_allocator(vm);
        buf = allocator_alloc(alloc, underlying_size);
        int64_t v = auto_val;
        memcpy(buf, &v, (size_t)underlying_size);
      }

      value_t r = enum_type_add_item(vm, et, item_name, buf);
      if (buf)
        allocator_free(vm_get_allocator(vm), &buf);
      if (value_is_abnormal(r))
        return r;

      auto_val++;
    }
  }

  /* return the type value — anonymous enum is a type expression */
  return type_val;
}
