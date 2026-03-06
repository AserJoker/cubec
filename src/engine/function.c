#include "engine/function.h"
#include "core/allocator.h"

static void cubec_function_meta_dispose(cubec_function_meta_t self,
                                        cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->args);
}

cubec_function_meta_t cubec_create_function_meta(cubec_allocator_t allocator,
                                                 cubec_array_t args,
                                                 cubec_type_t type,
                                                 bool variadic) {
  cubec_function_meta_t meta =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_function_meta_t),
                            (cubec_dispose_fn_t)cubec_function_meta_dispose);
  meta->args = args;
  meta->type = type;
  meta->variadic = variadic;
  return meta;
}
cubec_function_desc_t
cubec_create_comptime_function_desc(cubec_allocator_t allocator,
                                    cubec_ast_node_t node) {
  cubec_function_desc_t desc = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_function_desc_t), NULL);
  desc->kind = CUBEC_FUNCTION_COMPTIME;
  desc->node = node;
  return desc;
}
cubec_function_desc_t
cubec_create_runtime_function_desc(cubec_allocator_t allocator) {
  cubec_function_desc_t desc = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_function_desc_t), NULL);
  desc->kind = CUBEC_FUNCTION_RUNTIME;
  return desc;
}
cubec_function_desc_t
cubec_create_native_function_desc(cubec_allocator_t allocator,
                                  cubec_native_handle_fn_t handle) {
  cubec_function_desc_t desc = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_function_desc_t), NULL);
  desc->kind = CUBEC_FUNCTION_NATIVE;
  desc->handle = handle;
  return desc;
}