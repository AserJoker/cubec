#include "engine/type.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/ptr.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
struct _type_t {
  type_kind_t kind;
  size_t size;
  size_t align;
  void *meta;
  struct _type_operator_t opts;
};
static void type_dispose(type_t self, allocator_t allocator) {
  allocator_free(allocator, self->meta);
}
static value_t value_ref_default(value_t self, context_t ctx) {
  type_t type = value_get_type(self);
  value_t vptr_type = create_ptr_type(ctx, type, true, false);
  type_t ptr_type = *(type_t *)value_get_data(vptr_type);
  void *data = value_get_data(self);
  bool mutable = value_is_mutable(self);
  if (data) {
    return context_create_value(ctx, ptr_type, mutable, &data, NULL);
  } else {
    return context_create_value(ctx, ptr_type, mutable, NULL, NULL);
  }
}
type_t create_type(allocator_t allocator, type_kind_t kind, size_t size,
                   size_t align, void *meta, type_operator_t opt) {
  type_t self = allocator_alloc(allocator, sizeof(struct _type_t),
                                (dispose_fn_t)type_dispose);
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
    self->opts.ref = value_ref_default;
  }
  return self;
}
type_kind_t type_get_kind(type_t self) { return self->kind; }
type_operator_t type_get_operator(type_t self) { return &self->opts; }
size_t type_get_size(type_t self) { return self->size; }
void type_set_size(type_t self, size_t size) { self->size = size; }
void *type_get_meta(type_t self) { return self->meta; }
size_t type_get_align(type_t self) { return self->align; }
void type_set_align(type_t self, size_t align) { self->align = align; }

bool type_is_equal(type_t self, type_t another) {
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
const char *type_kind_to_string(type_kind_t kind) { return kind_name[kind]; }
char *type_to_string(type_t self, allocator_t allocator) {
  if (self->opts.type_to_string) {
    return self->opts.type_to_string(self, allocator);
  }
  size_t len = snprintf(
      NULL, 0, "type{align = %" PRIuPTR ", size = %" PRIuPTR ", kind = %s}",
      self->align, self->size, kind_name[self->kind]);
  char *str = allocator_alloc(allocator, len + 1, NULL);
  sprintf(str, "type{align = %" PRIuPTR ", size = %" PRIuPTR ", kind = %s}",
          self->align, self->size, kind_name[self->kind]);
  return str;
}
struct _value_t *create_type_value(struct _context_t *ctx, type_t type,
                                   bool mutable, const char *name) {
  value_t vtype = context_load(ctx, "type");
  type_t ttype = *(type_t *)value_get_data(vtype);
  return context_create_value(ctx, ttype, mutable, &type, name);
}