#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/diagnostic.h"
#include "engine/value.h"
#include "engine/type.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/enum_type.h"
#include "engine/integer_type.h"
#include "cubec/statement_enum.h"
#include "cubec/enum_item.h"
#include "cubec/literal_identifier.h"
#include "core/string.h"
#include "core/vec.h"
#include <string.h>

/* ---- helper: extract name string from identifier node ---- */

static const char *_get_name(node_t identifier) {
  cubec_literal_identifier_t id = (cubec_literal_identifier_t)identifier;
  return string_get(id->value);
}

/* ---- helper: bind a name to an already-registered value ---- */

static void _bind_name(vm_t vm, value_t val, const char *name) {
  scope_t scope = vm_get_current_scope(vm);
  if (scope && name) {
    name_t n = name_create(scope->allocator, val);
    char *owned = cstring_clone(scope->allocator, name);
    strmap_insert(scope->names, owned, n);
    allocator_free(scope->allocator, &owned);
  }
}

/* ---- helper: determine the underlying type for an enum ----
 * If all items specify the same type, use that; otherwise default to i32.
 * Items may have no type annotation, in which case they inherit from
 * the first typed item or fall back to i32. */

static type_t _resolve_underlying(vm_t vm, vec_t items) {
  type_t resolved = NULL;
  size_t count = vec_get_size(items);

  for (size_t i = 0; i < count; i++) {
    cubec_enum_item_t item = (cubec_enum_item_t)vec_get(items, i);
    if (!item->type) continue;

    value_t type_val = run_expression(vm, item->type, false);
    if (value_is_abnormal(type_val))
      return NULL;

    if (type_get_kind(value_get_type(type_val)) != TYPE_KIND_TYPE)
      return NULL;

    type_t t = (type_t)value_get_data(type_val);
    if (!resolved) {
      resolved = t;
    } else if (type_get_kind(resolved) != type_get_kind(t) ||
               type_get_size(resolved) != type_get_size(t)) {
      return NULL;
    }
  }

  /* default to i32 */
  if (!resolved) {
    value_t i32_val = vm_get_i32_type(vm);
    resolved = (type_t)value_get_data(i32_val);
  }
  return resolved;
}

/* ---- helper: wrap a typed integer value as a type value ---- */

static value_t _type_to_value(vm_t vm, type_t t) {
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
  return vm_create_value_ref(vm, type_type, (const void *)t, NULL);
}

/* ---- main entry ---- */

value_t run_statement_enum(vm_t vm, node_t node, bool shadow) {
  (void)shadow; /* enum type expressions evaluate identically in both modes */
  cubec_statement_enum_t stmt = (cubec_statement_enum_t)node;
  const char *name = _get_name(stmt->name);
  size_t item_count = vec_get_size(stmt->items);

  /* resolve the underlying type */
  type_t underlying = _resolve_underlying(vm, stmt->items);
  if (!underlying)
    return create_exception_value(vm,
        "enum '%s' has inconsistent or invalid item types", name);

  /* create the enum type value */
  value_t underlying_type_val = _type_to_value(vm, underlying);
  value_t type_val = vm_create_enum_type_value(vm, name, underlying_type_val,
                                                false, vm_get_current_module_id(vm));
  enum_type_t et = (enum_type_t)value_get_data(type_val);
  uint64_t underlying_size = type_get_size(underlying);

  /* auto-increment counter (stored as i64 to cover all integer types) */
  int64_t auto_val = 0;

  for (size_t i = 0; i < item_count; i++) {
    cubec_enum_item_t item = (cubec_enum_item_t)vec_get(stmt->items, i);
    const char *item_name = _get_name(item->name);

    if (item->value) {
      /* evaluate the explicit value expression */
      value_t val = run_expression(vm, item->value, false);
      if (value_is_abnormal(val))
        return val;

      /* safe_cast to the underlying type */
      value_t cast_val = value_safe_cast(vm, val, underlying);
      if (value_is_abnormal(cast_val))
        return create_exception_value(vm,
            "enum '%s' item '%s' value cannot be converted to underlying type '%s'",
            name, item_name, type_get_name(underlying));

      /* add item with the cast value's data */
      value_t r = enum_type_add_item(vm, et, item_name, value_get_data(cast_val));
      if (value_is_abnormal(r))
        return r;

      /* update auto-increment from the explicit value */
      if (underlying_size > 0)
        memcpy(&auto_val, value_get_data(cast_val), (size_t)underlying_size);
      auto_val++;
    } else {
      /* auto-increment: create an underlying-type value from auto_val */
      void *buf = NULL;
      if (underlying_size > 0) {
        allocator_t alloc = vm_get_allocator(vm);
        buf = allocator_alloc(alloc, underlying_size);
        int64_t v = auto_val;
        /* truncate to underlying size — handles i8/u8/i16/u16/i32/u32/i64/u64 */
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

  /* bind the name in current scope */
  _bind_name(vm, type_val, name);

  return create_void_value(vm);
}
