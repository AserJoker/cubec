#include "engine/ptr.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
#include <string.h>
struct _ptr_meta_t {
  type_t type;
  bool mutable;
  bool volatile_;
};
typedef struct _ptr_meta_t *ptr_meta_t;
static ptr_meta_t create_ptr_meta(allocator_t allocator, type_t type,
                                  bool mutable, bool volatile_) {
  ptr_meta_t self =
      allocator_alloc(allocator, sizeof(struct _ptr_meta_t), NULL);
  self->mutable = mutable;
  self->volatile_ = volatile_;
  self->type = type;
  return self;
}
type_t ptr_type_get_type(type_t self) {
  ptr_meta_t meta = type_get_meta(self);
  return meta->type;
}
bool ptr_type_is_mutable(type_t self) {
  ptr_meta_t meta = type_get_meta(self);
  return meta->mutable;
}
bool ptr_type_is_volatile(type_t self) {
  ptr_meta_t meta = type_get_meta(self);
  return meta->volatile_;
}
static bool ptr_type_is_equal(type_t self, type_t another) {
  ptr_meta_t self_meta = type_get_meta(self);
  ptr_meta_t another_meta = type_get_meta(another);
  if (self_meta->volatile_ != another_meta->volatile_) {
    return false;
  }
  if (self_meta->mutable != another_meta->mutable) {
    return false;
  }
  return type_is_equal(self_meta->type, another_meta->type);
}
static char *ptr_type_to_string(type_t self, allocator_t allocator) {
  ptr_meta_t meta = type_get_meta(self);
  char *base_str = type_to_string(meta->type, allocator);
  size_t len = strlen(base_str) + 32;
  size_t offset = 0;
  char *str = allocator_alloc(allocator, len, NULL);
  if (type_get_kind(self) == CUBEC_VALUE_TYPE_PTR) {
    str[offset++] = '*';
  } else {
    strcpy(&str[offset], "[*]");
  }
  if (!meta->mutable) {
    strcpy(&str[offset], "const ");
    offset += 6;
  }
  if (meta->volatile_) {
    strcpy(&str[offset], "volatile ");
    offset += 9;
  }
  strcpy(&str[offset], base_str);
  offset += strlen(base_str);
  allocator_free(allocator, base_str);
  str[offset] = 0;
  return str;
}

static value_t ptr_unref(value_t self, context_t ctx) {
  type_t ptr_type = value_get_type(self);
  type_t type = ptr_type_get_type(ptr_type);
  void **data = (void **)value_get_data(self);
  void *dst = NULL;
  if (data) {
    dst = *data;
  }
  bool mutable = value_is_mutable(self);
  return context_create_value(ctx, type, mutable, dst, NULL);
}
static value_t ptr_convert(value_t self, context_t ctx, type_t type) {
  if (type_get_kind(type) == CUBEC_VALUE_TYPE_OPAQUE) {
    void *data = (void *)value_get_data(self);
    return context_create_value(ctx, type, false, data, NULL);
  }
  type_t ptr_type = value_get_type(self);
  if (type_get_kind(type) == type_get_kind(ptr_type)) {
    type_t src_type = ptr_type_get_type(ptr_type);
    type_t dst_type = ptr_type_get_type(type);
    if (ptr_type_is_equal(src_type, dst_type)) {
      void *data = value_get_data(self);
      return context_create_value(ctx, type, false, data, NULL);
    }
  }
  allocator_t allocator = context_get_allocator(ctx);
  char *dst_type_name = type_to_string(type, allocator);
  char *src_type_name = type_to_string(ptr_type, allocator);
  value_t err = create_error(ctx, "cannot convert '%s' to '%s'", src_type_name,
                             dst_type_name);
  allocator_free(allocator, src_type_name);
  allocator_free(allocator, dst_type_name);
  return err;
}
value_t create_ptr_type(context_t ctx, type_t type, bool mutable,
                        bool volatile_) {
  ptr_meta_t meta =
      create_ptr_meta(context_get_allocator(ctx), type, mutable, volatile_);
  struct _type_operator_t opt = {
      .is_type_equal = ptr_type_is_equal,
      .type_to_string = ptr_type_to_string,
      .unref = ptr_unref,
      .convert = ptr_convert,
  };
  return context_create_type(ctx, CUBEC_VALUE_TYPE_PTR, sizeof(void *),
                             sizeof(void *), meta, &opt, NULL);
}
value_t create_ptr_array_type(context_t ctx, type_t type, bool mutable,
                              bool volatile_) {
  ptr_meta_t meta =
      create_ptr_meta(context_get_allocator(ctx), type, mutable, volatile_);
  struct _type_operator_t opt = {
      .is_type_equal = ptr_type_is_equal,
      .type_to_string = ptr_type_to_string,
      .convert = ptr_convert,
  };
  return context_create_type(ctx, CUBEC_VALUE_TYPE_PARRAY, sizeof(void *),
                             sizeof(void *), meta, &opt, NULL);
}