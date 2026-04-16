#include "engine/type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "engine/array.h"
#include "engine/context.h"
#include "engine/ptr.h"
#include "engine/struct.h"
#include "engine/union.h"
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
  type_t ptr;
};
static void type_dispose(type_t self, allocator_t allocator) {
  allocator_free(allocator, self->meta);
}
static value_t value_ref_default(value_t self, context_t ctx) {
  type_t type = value_get_type(self);
  type_t ptr_type = type_get_ptr_type(type, ctx);
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
bool type_is_safe_convert(type_t ltype, type_t rtype) {
  if (type_is_equal(ltype, rtype)) {
    return true;
  }
  type_kind_t lkind = type_get_kind(ltype);
  type_kind_t rkind = type_get_kind(rtype);
  if (lkind >= VALUE_TYPE_INT8 && lkind <= VALUE_TYPE_INT64) {
    if (rkind >= VALUE_TYPE_INT8 && rkind <= VALUE_TYPE_INT64) {
      return true;
    }
  }
  if (lkind >= VALUE_TYPE_UINT8 && lkind <= VALUE_TYPE_UINT64) {
    if (rkind >= VALUE_TYPE_UINT8 && rkind <= VALUE_TYPE_UINT64) {
      return true;
    }
  }
  if (lkind >= VALUE_TYPE_FLOAT32 && lkind <= VALUE_TYPE_FLOAT64) {
    if (rkind >= VALUE_TYPE_FLOAT32 && rkind <= VALUE_TYPE_FLOAT64) {
      return true;
    }
  }
  if (lkind == VALUE_TYPE_PTR || lkind == VALUE_TYPE_PARRAY) {
    if (rkind == VALUE_TYPE_OPAQUE) {
      return true;
    }
  }
  if (lkind == VALUE_TYPE_ARRAY && rkind == VALUE_TYPE_PARRAY) {
    type_t larr_type = array_type_get_type(ltype);
    type_t rarr_type = ptr_type_get_type(rtype);
    return type_is_equal(larr_type, rarr_type);
  }
  if (lkind == VALUE_TYPE_NULL &&
      (rkind == VALUE_TYPE_PTR || rkind == VALUE_TYPE_PARRAY ||
       rkind == VALUE_TYPE_OPAQUE)) {
    return true;
  }
  if (lkind == VALUE_TYPE_PTR && rkind == VALUE_TYPE_PTR) {
    type_t lptr_type = ptr_type_get_type(ltype);
    type_t rptr_type = ptr_type_get_type(rtype);
    if (type_get_kind(lptr_type) == VALUE_TYPE_STRUCT &&
        type_get_kind(rptr_type) == VALUE_TYPE_STRUCT) {
      array_t lfields = struct_type_get_fields(lptr_type);
      array_t rfields = struct_type_get_fields(lptr_type);
      if (array_get_size(lfields) != array_get_size(lfields)) {
        return false;
      }
      for (size_t idx = 0; idx < array_get_size(lfields); idx++) {
        struct_field_t field = array_get(lfields, idx);
        struct_field_t rfield = array_get(rfields, idx);
        if (field->offset != rfield->offset) {
          return false;
        }
        if (strcmp(field->name, rfield->name) != 0) {
          return false;
        }
        if (type_is_equal(field->type, rfield->type)) {
          return false;
        }
      }
      return true;
    }
    if (type_get_kind(lptr_type) == VALUE_TYPE_UNION &&
        type_get_kind(rptr_type) == VALUE_TYPE_UNION) {
      array_t lfields = union_type_get_fields(lptr_type);
      array_t rfields = union_type_get_fields(lptr_type);
      if (array_get_size(lfields) != array_get_size(lfields)) {
        return false;
      }
      for (size_t idx = 0; idx < array_get_size(lfields); idx++) {
        union_field_t field = array_get(lfields, idx);
        union_field_t rfield = array_get(rfields, idx);
        if (strcmp(field->name, rfield->name) != 0) {
          return false;
        }
        if (type_is_equal(field->type, rfield->type)) {
          return false;
        }
      }
      return true;
    }
    return type_is_equal(lptr_type, rptr_type);
  }
  return false;
}

static const char *kind_name[] = {

    "error",   // VALUE_TYPE_ERROR,
    "any",     // VALUE_TYPE_ANY,
    "builtin", // VALUE_TYPE_BUILTIN,
    "void",    // VALUE_TYPE_VOID,
    "type",    // VALUE_TYPE_TYPE,
    "bool",    // VALUE_TYPE_BOOL,
    "i8",      // VALUE_TYPE_INT8,
    "i16",     // VALUE_TYPE_INT16,
    "i32",     // VALUE_TYPE_INT32,
    "i64",     // VALUE_TYPE_INT64,
    "u8",      // VALUE_TYPE_UINT8,
    "u16",     // VALUE_TYPE_UINT16,
    "u32",     // VALUE_TYPE_UINT32,
    "u64",     // VALUE_TYPE_UINT64,
    "f32",     // VALUE_TYPE_FLOAT32,
    "f64",     // VALUE_TYPE_FLOAT64,
    "str",     // VALUE_TYPE_STR,
    "ptr",     // VALUE_TYPE_PTR,
    "parray",  // VALUE_TYPE_PARRAY,
    "opaque",  // VALUE_TYPE_OPAQUE,
    "array",   // VALUE_TYPE_ARRAY,
    "struct",  // VALUE_TYPE_STRUCT,
    "union",   // VALUE_TYPE_UNION,
    "function" // VALUE_TYPE_FUNCTION,
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
  value_t val = context_create_value(ctx, ttype, mutable, &type, name);
  value_set_comptime(val, true);
  return val;
}

type_t type_get_ptr_type(type_t self, struct _context_t *ctx) {
  if (!self->ptr) {
    value_t vtype = create_ptr_type(ctx, self, true, false);
    self->ptr = *(type_t *)value_get_data(vtype);
  }
  return self->ptr;
}