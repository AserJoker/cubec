#include "engine/function.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
static void cubec_function_meta_dispose(cubec_function_meta_t self,
                                        cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->args);
}
cubec_function_meta_t cubec_create_function_meta(cubec_allocator_t allocator,
                                                 cubec_type_t type,
                                                 size_t num_args,
                                                 cubec_type_t *args,
                                                 bool is_variadic) {
  cubec_function_meta_t meta =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_function_meta_t),
                            (cubec_dispose_fn_t)cubec_function_meta_dispose);
  meta->type = type;
  meta->args = cubec_create_array(allocator, NULL);
  for (size_t idx = 0; idx < num_args; idx++) {
    cubec_array_push(meta->args, args[idx]);
  }
  meta->is_variadic = is_variadic;
  return meta;
}
static void cubec_function_desc_dispose(cubec_function_desc_t self,
                                        cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->name);
}
cubec_function_desc_t cubec_create_function_desc(cubec_allocator_t allocator,
                                                 cubec_function_kind_t kind,
                                                 void *ptr, const char *name) {
  cubec_function_desc_t desc =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_function_desc_t),
                            (cubec_dispose_fn_t)cubec_function_desc_dispose);
  desc->kind = kind;
  desc->ptr = ptr;
  if (name) {
    desc->name = cubec_create_cstring(allocator, name);
  } else {
    desc->name = NULL;
  }
  return desc;
}