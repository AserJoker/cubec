#include "engine/ptr.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/type.h"
struct _cubec_ptr_meta_t {
  cubec_type_t type;
  bool mutable;
  bool volatile_;
};
typedef struct _cubec_ptr_meta_t *cubec_ptr_meta_t;
cubec_ptr_meta_t cubec_create_ptr_meta(cubec_allocator_t allocator,
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
cubec_type_t cubec_create_ptr_type(cubec_context_t ctx, cubec_type_t type,
                                   bool mutable, bool volatile_) {
  cubec_ptr_meta_t meta = cubec_create_ptr_meta(
      cubec_context_get_allocator(ctx), type, mutable, volatile_);
  return cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_PTR, sizeof(void *),
                                   sizeof(void *), meta);
}
cubec_type_t cubec_create_ptr_array_type(cubec_context_t ctx, cubec_type_t type,
                                         bool mutable, bool volatile_) {
  cubec_ptr_meta_t meta = cubec_create_ptr_meta(
      cubec_context_get_allocator(ctx), type, mutable, volatile_);
  return cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_PTR_ARRAY,
                                   sizeof(void *), sizeof(void *), meta);
}