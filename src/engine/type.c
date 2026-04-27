#include "engine/type.h"
#include "core/allocator.h"
#include "core/string.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/ptr.h"
#include "engine/struct.h"
#include "engine/value.h"
#include <stdbool.h>
#include <string.h>
typedef struct _type_data_t *type_data_t;
struct _type_t {
  type_kind_t kind;
  size_t size;
  size_t align;
  char *name;
  char *id;
  type_operator_t opt;
  void *meta;
};
static void type_dispose(type_t self, allocator_t allocator) {
  allocator_free(allocator, self->name);
  allocator_free(allocator, self->id);
  allocator_free(allocator, self->meta);
}
type_t create_type(allocator_t allocator, type_kind_t kind, size_t size,
                   size_t align, const char *name, const char *id,
                   type_operator_t *opt, void *meta) {
  type_t self = allocator_alloc(allocator, sizeof(struct _type_t),
                                (dispose_fn_t)type_dispose);
  self->kind = kind;
  self->size = size;
  self->align = align;
  self->name = NULL;
  if (name) {
    self->name = create_cstring(allocator, name);
  }
  self->id = create_cstring(allocator, id);
  if (opt) {
    self->opt = *opt;
  } else {
    memset(&self->opt, 0, sizeof(type_operator_t));
  }
  self->meta = meta;
  return self;
}
type_kind_t type_get_kind(type_t self) { return self->kind; }
size_t type_get_size(type_t self) { return self->size; }
size_t type_get_align(type_t self) { return self->align; }
void type_set_size(type_t self, size_t size) { self->size = size; }
void type_set_align(type_t self, size_t align) { self->align = align; }
const char *type_get_name(type_t self) {
  return self->name ? self->name : "(nonamed)";
}
const char *type_get_id(type_t self) { return self->id; }
const type_operator_t *type_get_operator(type_t self) { return &self->opt; }
void *type_get_meta(type_t self) { return self->meta; }

static value_t type_eq(value_t self, context_t ctx, value_t another) {
  type_t type = value_get_type(another);
  type_t t = value_get_type(self);
  if (type_get_kind(type) != TYPE_KIND_TYPE) {
    another = value_safe_convert(another, ctx, t);
    type = value_get_type(another);
    if (type_get_kind(type) == TYPE_KIND_ERROR) {
      return another;
    }
  }
  type_t t1 = *(type_t *)value_get_data(self);
  type_t t2 = *(type_t *)value_get_data(another);
  return create_comptime_bool(ctx, strcmp(t1->id, t2->id) == 0, false, NULL);
}

static value_t type_ne(value_t self, context_t ctx, value_t another) {
  type_t type = value_get_type(another);
  type_t t = value_get_type(self);
  if (type_get_kind(type) != TYPE_KIND_TYPE) {
    another = value_safe_convert(another, ctx, t);
    type = value_get_type(another);
    if (type_get_kind(type) == TYPE_KIND_ERROR) {
      return another;
    }
  }
  type_t t1 = *(type_t *)value_get_data(self);
  type_t t2 = *(type_t *)value_get_data(another);
  return create_comptime_bool(ctx, strcmp(t1->id, t2->id) != 0, false, NULL);
}

static value_t type_get_field(value_t self, context_t ctx, const char *name) {
  type_t type = *(type_t *)value_get_data(self);
  if (type_get_kind(type) == TYPE_KIND_STRUCT) {
    struct_attribute_t attr = struct_type_get_attribute(type, name);
    if (attr) {
      return attr->value;
    }
  }
  return create_error(ctx, "no member '%s' in type '%s'", name,
                      type_get_name(type));
}

static value_t type_set_field(value_t self, context_t ctx, const char *name,
                              value_t value) {
  type_t type = *(type_t *)value_get_data(self);
  if (type_get_kind(type) == TYPE_KIND_STRUCT) {
    struct_attribute_t attr = struct_type_get_attribute(type, name);
    if (attr) {
      return value_assigment(attr->value, ctx, value);
    }
  }
  return create_error(ctx, "no member '%s' in type '%s'", name,
                      type_get_name(type));
}

static value_t type_deref(value_t self, context_t ctx) {
  type_t type = *(type_t *)value_get_data(self);
  type_t ptr_type = create_ptr_type(ctx, type, true, false);
  return create_type_value(ctx, ptr_type, false, NULL);
}

static char *type_write_ast(value_t self, allocator_t allocator) {
  type_t type = *(type_t *)value_get_data(self);
  return create_cstring(allocator, type->id);
}

void type_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  type_operator_t opt = {
      .type_eq = type_default_eq,
      .addr_of = value_default_address_of,
      .deref = type_deref,
      .get_field = type_get_field,
      .set_field = type_set_field,
      .eq = type_eq,
      .ne = type_ne,
      .assigment = value_default_assigment,
      .write_ast = type_write_ast,
  };
  type_t type = create_type(allocator, TYPE_KIND_TYPE, sizeof(type_t),
                            sizeof(type_t), "type", "type", &opt, NULL);
  context_store_type(ctx, type);
  context_create_value(ctx, type, &type, false, true, "type");
}

value_t create_type_value(context_t ctx, type_t data, bool mut,
                          const char *name) {
  allocator_t allocator = context_get_allocator(ctx);
  value_t vtype = context_load(ctx, "type");
  type_t type = *(type_t *)value_get_data(vtype);
  return context_create_value(ctx, type, &data, mut, true, name);
}
bool type_default_eq(type_t self, type_t another) {
  return strcmp(self->id, another->id) == 0;
}
bool type_is_equal(type_t self, type_t another) {
  type_eq_fn_t fn = self->opt.type_eq;
  return fn(self, another);
}