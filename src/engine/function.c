#include "engine/function.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
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
static bool cubec_function_is_equal(cubec_type_t self, cubec_type_t another) {
  cubec_function_meta_t self_meta = cubec_type_get_meta(self);
  cubec_function_meta_t another_meta = cubec_type_get_meta(another);
  if (self_meta->variadic != another_meta->variadic) {
    return false;
  }
  if (cubec_array_get_size(self_meta->arguments) !=
      cubec_array_get_size(another_meta->arguments)) {
    return false;
  }
  if (!cubec_type_is_equal(self_meta->type, another_meta->type)) {
    return false;
  }
  for (size_t idx = 0; idx < cubec_array_get_size(self_meta->arguments);
       idx++) {
    cubec_type_t self_arg = cubec_array_get(self_meta->arguments, idx);
    cubec_type_t another_arg = cubec_array_get(another_meta->arguments, idx);
    if (!cubec_type_is_equal(self_arg, another_arg)) {
      return false;
    }
  }
  return true;
}
static char *cubec_function_type_to_string(cubec_type_t self,
                                           cubec_allocator_t allocator) {
  cubec_function_meta_t meta = cubec_type_get_meta(self);
  size_t len = 32;
  size_t argc = cubec_array_get_size(meta->arguments);
  char *argv[argc];
  for (size_t idx = 0; idx < argc; idx++) {
    cubec_type_t arg = cubec_array_get(meta->arguments, idx);
    char *arg_str = cubec_type_to_string(arg, allocator);
    if (idx == argc - 1 && meta->variadic) {
      char *arg_str_var =
          cubec_allocator_alloc(allocator, strlen(arg_str) + 3, NULL);
      sprintf(arg_str_var, "...%s", arg_str);
      cubec_allocator_free(allocator, arg_str);
      arg_str = arg_str_var;
    }
    argv[idx] = arg_str;
    len += strlen(arg_str) + 2;
  }
  char *type_str = cubec_type_to_string(meta->type, allocator);
  len += strlen(type_str);
  size_t offset = 0;
  char *str = cubec_allocator_alloc(allocator, len, NULL);
  strcpy(&str[offset], "func(");
  offset += 5;
  for (size_t idx = 0; idx < argc; idx++) {
    if (idx != 0) {
      strcpy(&str[offset], ", ");
      offset += 2;
    }
    strcpy(&str[offset], argv[idx]);
    offset += strlen(argv[idx]);
    cubec_allocator_free(allocator, argv[idx]);
  }
  str[offset++] = ')';
  str[offset++] = ':';
  str[offset++] = ' ';
  strcpy(&str[offset], type_str);
  offset += strlen(type_str);
  str[offset] = 0;
  cubec_allocator_free(allocator, type_str);
  return str;
}
static cubec_value_t cubec_function_call(cubec_value_t self,
                                         cubec_context_t ctx, size_t argc,
                                         cubec_value_t argv[]) {
  return cubec_create_error(ctx, "not implement");
}

cubec_value_t cubec_create_function_type(cubec_context_t ctx, cubec_type_t type,
                                         size_t num_args, cubec_type_t args[],
                                         bool variadic) {
  cubec_function_meta_t meta = cubec_create_function_meta(
      cubec_context_get_allocator(ctx), type, num_args, args, variadic);
  struct _cubec_type_operator_t opt = {
      .is_type_equal = cubec_function_is_equal,
      .type_to_string = cubec_function_type_to_string,
      .call = cubec_function_call,
  };
  return cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_FUNCTION,
                                   sizeof(void *), sizeof(void *), meta, &opt,
                                   NULL);
}
cubec_array_t cubec_function_type_get_arguments(cubec_type_t self) {
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

cubec_value_t cubec_create_function(cubec_context_t ctx, cubec_type_t func_type,
                                    cubec_ast_node_t func, bool mutable,
                                    const char *name) {
  return cubec_context_create_value(ctx, func_type, mutable, func, name);
}