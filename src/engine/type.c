#include "engine/type.h"
#include "core/allocator.h"
#include "core/string.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/struct.h"
#include "engine/value.h"
#include <stdbool.h>

ctype_t create_ctype(allocator_t allocator, type_t type, bool mut) {
  ctype_t ctype = allocator_alloc(allocator, sizeof(struct _ctype_t), NULL);
  ctype->type = type;
  ctype->mut = mut;
  return ctype;
}
static void type_dispose(type_t self, allocator_t allocator) {
  allocator_free(allocator, self->id);
  allocator_free(allocator, self->name);
  allocator_free(allocator, self->meta);
}
type_t create_type(allocator_t allocator, type_kind_t kind, const char *name,
                   const char *id, size_t size, size_t align,
                   type_operator_t opt, void *meta) {
  type_t self = allocator_alloc(allocator, sizeof(struct _type_t),
                                (dispose_fn_t)type_dispose);
  self->align = align;
  self->id = create_cstring(allocator, id);
  self->name = create_cstring(allocator, name);
  self->kind = kind;
  self->size = size;
  if (opt) {
    self->opt = *opt;
  } else {
    self->opt = (struct _type_operator_t){};
  }
  self->meta = meta;
  return self;
}
bool type_is_equal(type_t self, type_t another) {
  if (self->opt.type_equal) {
    return self->opt.type_equal(self, another);
  }
  return self->kind == another->kind;
}

static value_t type_eq(value_t self, context_t ctx, value_t another) {
  if (another->type->kind == TYPE_KIND_TYPE) {
    return create_comptime_bool(
        ctx, type_is_equal(*(type_t *)self->type, *(type_t *)another->type),
        false, NULL);
  }
  return NULL;
}
static value_t type_ne(value_t self, context_t ctx, value_t another) {
  if (another->type->kind == TYPE_KIND_TYPE) {
    return create_comptime_bool(
        ctx, !type_is_equal(*(type_t *)self->type, *(type_t *)another->type),
        false, NULL);
  }
  return NULL;
}
static value_t type_get_field(value_t self, context_t ctx, const char *field) {
  type_t type = *(type_t *)self->data;
  if (type->kind == TYPE_KIND_STRUCT) {
    struct_attribute_t attr = struct_type_get_attribute(type, field);
    if (!attr) {
      return create_error(ctx, "no member '%s' in '%s'", field, type->name);
    }
    return attr->value;
  }
  return NULL;
}
static value_t type_set_field(value_t self, context_t ctx, const char *field,
                              value_t value) {
  type_t type = *(type_t *)self->data;
  if (type->kind == TYPE_KIND_STRUCT) {
    struct_attribute_t attr = struct_type_get_attribute(type, field);
    if (!attr) {
      return create_error(ctx, "no member '%s' in '%s'", field, type->name);
    }
    return value_assigment(attr->value, ctx, value);
  }
  return NULL;
}
void init_type_type(context_t ctx) {
  struct _type_operator_t opt = {
      .opt_eq = type_eq,
      .opt_ne = type_ne,
      .get_field = type_get_field,
      .set_field = type_set_field,
  };
  type_t type = create_type(ctx->allocator, TYPE_KIND_TYPE, "type", "type",
                            sizeof(type_t), sizeof(type_t), &opt, NULL);
  context_store_type(ctx, type);
  create_type_value(ctx, type, false, "type");
}
struct _value_t *create_type_value(struct _context_t *ctx, type_t type,
                                   bool mut, const char *name) {
  type_t t = context_load_type(ctx, "type");
  return context_create_comptime_value(ctx, t, &type, mut, name);
}