#include "engine/type.h"
#include "core/allocator.h"
#include "core/string.h"
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