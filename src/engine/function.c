#include "engine/function.h"
#include "core/allocator.h"
#include "core/array.h"
#include "engine/context.h"
#include "engine/type.h"
struct _cubec_function_meta_t {
  cubec_array_t arguments;
  cubec_type_t type;
  bool variadic;
};
typedef struct _cubec_function_meta_t *cubec_function_meta_t;
static void cubec_function_meta_dispose(cubec_function_meta_t self,
                                        cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->arguments);
}
static cubec_function_meta_t
cubec_create_function_meta(cubec_allocator_t allocator, cubec_type_t type,
                           size_t num_args, cubec_type_t args[],
                           bool variadic) {
  cubec_function_meta_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_function_meta_t),
                            (cubec_dispose_fn_t)cubec_function_meta_dispose);
  self->type = type;
  self->variadic = variadic;
  self->arguments = cubec_create_array(allocator, NULL);
  cubec_array_resize(self->arguments, num_args);
  for (size_t idx = 0; idx < num_args; idx++) {
    cubec_array_push(self->arguments, args[idx]);
  }
  return self;
}

cubec_type_t cubec_create_function_type(cubec_context_t ctx, cubec_type_t type,
                                        size_t num_args, cubec_type_t args[],
                                        bool variadic) {
  cubec_function_meta_t meta = cubec_create_function_meta(
      cubec_context_get_allocator(ctx), type, num_args, args, variadic);
  return cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_FUNCTION,
                                   sizeof(void *), sizeof(void *), meta);
}
cubec_array_t cubec_function_type_get_arguments(cubec_type_t self,
                                                cubec_allocator_t allocator) {
  cubec_function_meta_t meta = cubec_type_get_meta(self);
  return meta->arguments;
}
cubec_type_t cubec_function_type_get_type(cubec_type_t self) {
  cubec_function_meta_t meta = cubec_type_get_meta(self);
  return meta->type;
}
bool cubec_function_type_is_variadic(cubec_type_t self) {
  cubec_function_meta_t meta = cubec_type_get_meta(self);
  return meta->variadic;
}