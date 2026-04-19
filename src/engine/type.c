#include "engine/type.h"
#include "core/allocator.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/value.h"
#include <stdbool.h>
#include <string.h>
typedef struct _type_data_t *type_data_t;
struct _type_data_t {
  type_kind_t kind;
  size_t size;
  size_t align;
  char *name;
  type_operator_t opt;
  void *meta;
  size_t ref;
};
struct _type_t {
  type_data_t data;
};
static void type_data_dispose(type_data_t self, allocator_t allocator) {
  allocator_free(allocator, self->name);
  allocator_free(allocator, self->meta);
}
static void type_dispose(type_t self, allocator_t allocator) {
  self->data->ref--;
  if (!self->data->ref) {
    allocator_free(allocator, self->data);
  }
}
type_t create_type(allocator_t allocator, type_kind_t kind, size_t size,
                   size_t align, const char *name, type_operator_t *opt,
                   void *meta) {
  type_data_t data = allocator_alloc(allocator, sizeof(struct _type_data_t),
                                     (dispose_fn_t)type_data_dispose);
  data->kind = kind;
  data->size = size;
  data->align = align;
  data->name = create_cstring(allocator, name);
  if (opt) {
    data->opt = *opt;
  } else {
    memset(&data->opt, 0, sizeof(type_operator_t));
  }
  data->meta = meta;
  type_t self = allocator_alloc(allocator, sizeof(struct _type_t),
                                (dispose_fn_t)type_dispose);
  data->ref = 1;
  self->data = data;
  return self;
}
type_kind_t type_get_kind(type_t self) { return self->data->kind; }
size_t type_get_size(type_t self) { return self->data->size; }
size_t type_get_align(type_t self) { return self->data->align; }
const char *type_get_name(type_t self) { return self->data->name; }
const type_operator_t *type_get_operator(type_t self) {
  return &self->data->opt;
}
void *type_get_meta(type_t self) { return self->data->meta; }

type_t type_create_ref(type_t self, allocator_t allocator) {
  self->data->ref++;
  type_t type = allocator_alloc(allocator, sizeof(struct _type_t),
                                (dispose_fn_t)type_dispose);
  type->data = self->data;
  return type;
}

void type_type_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  type_operator_t opt = {};
  type_t type = create_type(allocator, TYPE_KIND_TYPE, sizeof(type_t),
                            sizeof(type_t), "type", &opt, NULL);
  context_create_value(ctx, type, type, false, true, "type");
}

value_t create_type_value(context_t ctx, type_t data, bool mutable,
                          bool comptime, const char *name) {
  allocator_t allocator = context_get_allocator(ctx);
  value_t vtype = context_load(ctx, "type");
  type_t type = (type_t)value_get_data(vtype);
  return context_create_value(ctx, type, data, mutable, comptime, name);
}