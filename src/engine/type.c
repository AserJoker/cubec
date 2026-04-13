#include "engine/type.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/ptr.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
struct _cubec_type_t {
  cubec_type_kind_t kind;
  size_t size;
  size_t align;
  void *meta;
  struct _cubec_type_operator_t opts;
};
static void cubec_type_dispose(cubec_type_t self, cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->meta);
}
static cubec_value_t cubec_value_ref_default(cubec_value_t self,
                                             cubec_context_t ctx) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_value_t vptr_type = cubec_create_ptr_type(ctx, type, true, false);
  cubec_type_t ptr_type = *(cubec_type_t *)cubec_value_get_data(vptr_type);
  void *data = cubec_value_get_data(self);
  bool mutable = cubec_value_is_mutable(self);
  if (data) {
    return cubec_context_create_value(ctx, ptr_type, mutable, &data, NULL);
  } else {
    return cubec_context_create_value(ctx, ptr_type, mutable, NULL, NULL);
  }
}
cubec_type_t cubec_create_type(cubec_allocator_t allocator,
                               cubec_type_kind_t kind, size_t size,
                               size_t align, void *meta,
                               cubec_type_operator_t opt) {
  cubec_type_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_type_t),
                            (cubec_dispose_fn_t)cubec_type_dispose);
  self->kind = kind;
  self->meta = meta;
  self->size = size;
  self->align = align;
  if (opt) {
    self->opts = *opt;
  } else {
    memset(&self->opts, 0, sizeof(self->opts));
  }
  if (!self->opts.ref) {
    self->opts.ref = cubec_value_ref_default;
  }
  return self;
}
cubec_type_kind_t cubec_type_get_kind(cubec_type_t self) { return self->kind; }
cubec_type_operator_t cubec_type_get_operator(cubec_type_t self) {
  return &self->opts;
}
size_t cubec_type_get_size(cubec_type_t self) { return self->size; }
void cubec_type_set_size(cubec_type_t self, size_t size) { self->size = size; }
void *cubec_type_get_meta(cubec_type_t self) { return self->meta; }
size_t cubec_type_get_align(cubec_type_t self) { return self->align; }
void cubec_type_set_align(cubec_type_t self, size_t align) {
  self->align = align;
}

bool cubec_type_is_equal(cubec_type_t self, cubec_type_t another) {
  if (self->kind == another->kind && self->align == another->align &&
      self->size == another->size) {
    if (self->opts.is_type_equal) {
      return self->opts.is_type_equal(self, another);
    }
    return true;
  }
  return false;
}

static const char *kind_name[] = {

    "error",   // CUBEC_VALUE_TYPE_ERROR,
    "any",     // CUBEC_VALUE_TYPE_ANY,
    "builtin", // CUBEC_VALUE_TYPE_BUILTIN,
    "void",    // CUBEC_VALUE_TYPE_VOID,
    "type",    // CUBEC_VALUE_TYPE_TYPE,
    "bool",    // CUBEC_VALUE_TYPE_BOOL,
    "i8",      // CUBEC_VALUE_TYPE_INT8,
    "i16",     // CUBEC_VALUE_TYPE_INT16,
    "i32",     // CUBEC_VALUE_TYPE_INT32,
    "i64",     // CUBEC_VALUE_TYPE_INT64,
    "u8",      // CUBEC_VALUE_TYPE_UINT8,
    "u16",     // CUBEC_VALUE_TYPE_UINT16,
    "u32",     // CUBEC_VALUE_TYPE_UINT32,
    "u64",     // CUBEC_VALUE_TYPE_UINT64,
    "f32",     // CUBEC_VALUE_TYPE_FLOAT32,
    "f64",     // CUBEC_VALUE_TYPE_FLOAT64,
    "str",     // CUBEC_VALUE_TYPE_STR,
    "ptr",     // CUBEC_VALUE_TYPE_PTR,
    "parray",  // CUBEC_VALUE_TYPE_PARRAY,
    "opaque",  // CUBEC_VALUE_TYPE_OPAQUE,
    "array",   // CUBEC_VALUE_TYPE_ARRAY,
    "struct",  // CUBEC_VALUE_TYPE_STRUCT,
    "union",   // CUBEC_VALUE_TYPE_UNION,
    "function" // CUBEC_VALUE_TYPE_FUNCTION,
};
const char *cubec_type_kind_to_string(cubec_type_kind_t kind) {
  return kind_name[kind];
}
char *cubec_type_to_string(cubec_type_t self, cubec_allocator_t allocator) {
  if (self->opts.type_to_string) {
    return self->opts.type_to_string(self, allocator);
  }
  size_t len = snprintf(
      NULL, 0, "type{align = %" PRIuPTR ", size = %" PRIuPTR ", kind = %s}",
      self->align, self->size, kind_name[self->kind]);
  char *str = cubec_allocator_alloc(allocator, len + 1, NULL);
  sprintf(str, "type{align = %" PRIuPTR ", size = %" PRIuPTR ", kind = %s}",
          self->align, self->size, kind_name[self->kind]);
  return str;
}
struct _cubec_value_t *cubec_create_type_value(struct _cubec_context_t *ctx,
                                               cubec_type_t type, bool mutable,
                                               const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "type");
  cubec_type_t ttype = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, ttype, mutable, &type, name);
}