#include "engine/value.h"
#include "core/allocator.h"
#include "engine/type.h"
struct _value_t {
  type_t type;
  bool mutable;
  bool comptime;
  void *data;
};
static void value_dispose(value_t self, allocator_t allocator) {
  allocator_free(allocator, self->data);
  allocator_free(allocator, self->type);
}
value_t create_value(allocator_t allocator, type_t type, bool mutable,
                     void *data, bool comptime) {
  value_t self = allocator_alloc(allocator, sizeof(struct _value_t),
                                 (dispose_fn_t)value_dispose);
  self->type = type_create_ref(type, allocator);
  self->mutable = mutable;
  self->data = data;
  self->comptime = comptime;
  return self;
}
bool value_is_mutable(value_t value) { return value->mutable; }
bool value_is_comptime(value_t value) { return value->comptime; }
void *value_get_data(value_t value) { return value->data; }
type_t value_get_type(value_t value) { return value->type; }