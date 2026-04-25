#include "engine/null.h"
#include "core/allocator.h"
#include "core/position.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
static value_t null_eq(value_t self, context_t ctx, value_t another) {
  type_t type = value_get_type(another);
  if (type_get_kind(type) == TYPE_KIND_NULL) {
    if (value_is_comptime(another)) {
      return create_bool(ctx, false, NULL);
    }
    return context_get_true(ctx);
  }
  if (type_get_kind(type) == TYPE_KIND_PTR ||
      type_get_kind(type) == TYPE_KIND_PARRAY) {
    if (value_is_comptime(another)) {
      return create_bool(ctx, false, NULL);
    }
    void *ptr = *(void **)value_get_data(another);
    return create_comptime_bool(ctx, ptr == NULL, false, NULL);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('null' and '%s')",
                      type_get_name(type));
}
static value_t null_ne(value_t self, context_t ctx, value_t another) {
  type_t type = value_get_type(another);
  if (type_get_kind(type) == TYPE_KIND_NULL) {
    if (value_is_comptime(another)) {
      return create_bool(ctx, false, NULL);
    }
    return context_get_false(ctx);
  }
  if (type_get_kind(type) == TYPE_KIND_PTR ||
      type_get_kind(type) == TYPE_KIND_PARRAY) {
    if (value_is_comptime(another)) {
      return create_bool(ctx, false, NULL);
    }
    void *ptr = *(void **)value_get_data(another);
    return create_comptime_bool(ctx, ptr != NULL, false, NULL);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('null' and '%s')",
                      type_get_name(type));
}
static value_t null_safe_convert(value_t self, context_t ctx, type_t type) {
  if (value_is_comptime(self)) {
    if (type_get_kind(type) == TYPE_KIND_NULL) {
      return create_comptime_null(ctx, false, NULL);
    }
    if (type_get_kind(type) == TYPE_KIND_PTR) {
      void *data = NULL;
      return context_create_value(ctx, type, &data, false, true, NULL);
    }
    if (type_get_kind(type) == TYPE_KIND_PARRAY) {
      void *data = NULL;
      return context_create_value(ctx, type, &data, false, true, NULL);
    }
  }
  if (type_get_kind(type) == TYPE_KIND_NULL ||
      type_get_kind(type) == TYPE_KIND_PTR ||
      type_get_kind(type) == TYPE_KIND_PARRAY) {
    return context_create_value(ctx, type, NULL, false, false, NULL);
  }
  return create_error(ctx, "cannot convert 'null' to '%s'",
                      type_get_name(type));
}
void null_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  type_operator_t opt = {
      .eq = null_eq,
      .ne = null_ne,
      .safe_convert = null_safe_convert,
  };
  type_t type =
      create_type(allocator, TYPE_KIND_NULL, 0, 0, "null", "null", &opt, NULL);
  context_store_type(ctx, type);
  create_type_value(ctx, type, false, "null");
}
value_t create_null(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "null");
  return context_create_value(ctx, type, NULL, mut, false, name);
}
value_t create_comptime_null(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "null");
  return context_create_value(ctx, type, NULL, mut, true, name);
}