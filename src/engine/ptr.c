#include "engine/ptr.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
#include <string.h>
struct _cubec_ptr_meta_t {
  cubec_type_t type;
  bool mutable;
  bool volatile_;
};
typedef struct _cubec_ptr_meta_t *cubec_ptr_meta_t;
static cubec_ptr_meta_t cubec_create_ptr_meta(cubec_allocator_t allocator,
                                              cubec_type_t type, bool mutable,
                                              bool volatile_) {
  cubec_ptr_meta_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ptr_meta_t), NULL);
  self->mutable = mutable;
  self->volatile_ = volatile_;
  self->type = type;
  return self;
}
cubec_type_t cubec_ptr_type_get_type(cubec_type_t self) {
  cubec_ptr_meta_t meta = cubec_type_get_meta(self);
  return meta->type;
}
bool cubec_ptr_type_is_mutable(cubec_type_t self) {
  cubec_ptr_meta_t meta = cubec_type_get_meta(self);
  return meta->mutable;
}
bool cubec_ptr_type_is_volatile(cubec_type_t self) {
  cubec_ptr_meta_t meta = cubec_type_get_meta(self);
  return meta->volatile_;
}
static bool cubec_ptr_type_is_equal(cubec_type_t self, cubec_type_t another) {
  cubec_ptr_meta_t self_meta = cubec_type_get_meta(self);
  cubec_ptr_meta_t another_meta = cubec_type_get_meta(another);
  if (self_meta->volatile_ != another_meta->volatile_) {
    return false;
  }
  if (self_meta->mutable != another_meta->mutable) {
    return false;
  }
  return cubec_type_is_equal(self_meta->type, another_meta->type);
}
static char *cubec_ptr_type_to_string(cubec_type_t self,
                                      cubec_allocator_t allocator) {
  cubec_ptr_meta_t meta = cubec_type_get_meta(self);
  char *base_str = cubec_type_to_string(meta->type, allocator);
  size_t len = strlen(base_str) + 32;
  size_t offset = 0;
  char *str = cubec_allocator_alloc(allocator, len, NULL);
  if (cubec_type_get_kind(self) == CUBEC_VALUE_TYPE_PTR) {
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
  cubec_allocator_free(allocator, base_str);
  str[offset] = 0;
  return str;
}

static cubec_value_t cubec_ptr_unref(cubec_value_t self, cubec_context_t ctx) {
  cubec_type_t ptr_type = cubec_value_get_type(self);
  cubec_type_t type = cubec_ptr_type_get_type(ptr_type);
  void **data = (void **)cubec_value_get_data(self);
  void *dst = NULL;
  if (data) {
    dst = *data;
  }
  bool mutable = cubec_value_is_mutable(self);
  return cubec_context_create_value(ctx, type, mutable, dst, NULL);
}
static cubec_value_t cubec_ptr_convert(cubec_value_t self, cubec_context_t ctx,
                                       cubec_type_t type) {
  if (cubec_type_get_kind(type) == CUBEC_VALUE_TYPE_OPAQUE) {
    void *data = (void *)cubec_value_get_data(self);
    return cubec_context_create_value(ctx, type, false, data, NULL);
  }
  cubec_type_t ptr_type = cubec_value_get_type(self);
  if (cubec_type_get_kind(type) == cubec_type_get_kind(ptr_type)) {
    cubec_type_t src_type = cubec_ptr_type_get_type(ptr_type);
    cubec_type_t dst_type = cubec_ptr_type_get_type(type);
    if (cubec_ptr_type_is_equal(src_type, dst_type)) {
      void *data = cubec_value_get_data(self);
      return cubec_context_create_value(ctx, type, false, data, NULL);
    }
  }
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  char *dst_type_name = cubec_type_to_string(type, allocator);
  char *src_type_name = cubec_type_to_string(ptr_type, allocator);
  cubec_value_t err = cubec_create_error(ctx, "cannot convert '%s' to '%s'",
                                         src_type_name, dst_type_name);
  cubec_allocator_free(allocator, src_type_name);
  cubec_allocator_free(allocator, dst_type_name);
  return err;
}
cubec_value_t cubec_create_ptr_type(cubec_context_t ctx, cubec_type_t type,
                                    bool mutable, bool volatile_) {
  cubec_ptr_meta_t meta = cubec_create_ptr_meta(
      cubec_context_get_allocator(ctx), type, mutable, volatile_);
  struct _cubec_type_operator_t opt = {
      .is_type_equal = cubec_ptr_type_is_equal,
      .type_to_string = cubec_ptr_type_to_string,
      .unref = cubec_ptr_unref,
      .convert = cubec_ptr_convert,
  };
  return cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_PTR, sizeof(void *),
                                   sizeof(void *), meta, &opt, NULL);
}
cubec_value_t cubec_create_ptr_array_type(cubec_context_t ctx,
                                          cubec_type_t type, bool mutable,
                                          bool volatile_) {
  cubec_ptr_meta_t meta = cubec_create_ptr_meta(
      cubec_context_get_allocator(ctx), type, mutable, volatile_);
  struct _cubec_type_operator_t opt = {
      .is_type_equal = cubec_ptr_type_is_equal,
      .type_to_string = cubec_ptr_type_to_string,
      .convert = cubec_ptr_convert,
  };
  return cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_PARRAY, sizeof(void *),
                                   sizeof(void *), meta, &opt, NULL);
}