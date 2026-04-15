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
struct _function_meta_t {
  array_t arguments;
  type_t type;
  bool variadic;
};
typedef struct _function_meta_t *function_meta_t;
static void function_meta_dispose(function_meta_t self, allocator_t allocator) {
  allocator_free(allocator, self->arguments);
}
static function_meta_t create_function_meta(allocator_t allocator, type_t type,
                                            size_t num_args, type_t args[],
                                            bool variadic) {
  function_meta_t self =
      allocator_alloc(allocator, sizeof(struct _function_meta_t),
                      (dispose_fn_t)function_meta_dispose);
  self->type = type;
  self->variadic = variadic;
  self->arguments = create_array(allocator, NULL);
  array_resize(self->arguments, num_args);
  for (size_t idx = 0; idx < num_args; idx++) {
    array_push(self->arguments, args[idx]);
  }
  return self;
}
static bool function_is_equal(type_t self, type_t another) {
  function_meta_t self_meta = type_get_meta(self);
  function_meta_t another_meta = type_get_meta(another);
  if (self_meta->variadic != another_meta->variadic) {
    return false;
  }
  if (array_get_size(self_meta->arguments) !=
      array_get_size(another_meta->arguments)) {
    return false;
  }
  if (!type_is_equal(self_meta->type, another_meta->type)) {
    return false;
  }
  for (size_t idx = 0; idx < array_get_size(self_meta->arguments); idx++) {
    type_t self_arg = array_get(self_meta->arguments, idx);
    type_t another_arg = array_get(another_meta->arguments, idx);
    if (!type_is_equal(self_arg, another_arg)) {
      return false;
    }
  }
  return true;
}
static char *function_type_to_string(type_t self, allocator_t allocator) {
  function_meta_t meta = type_get_meta(self);
  size_t len = 32;
  size_t argc = array_get_size(meta->arguments);
  char *argv[argc];
  for (size_t idx = 0; idx < argc; idx++) {
    type_t arg = array_get(meta->arguments, idx);
    char *arg_str = type_to_string(arg, allocator);
    if (idx == argc - 1 && meta->variadic) {
      char *arg_str_var = allocator_alloc(allocator, strlen(arg_str) + 3, NULL);
      sprintf(arg_str_var, "...%s", arg_str);
      allocator_free(allocator, arg_str);
      arg_str = arg_str_var;
    }
    argv[idx] = arg_str;
    len += strlen(arg_str) + 2;
  }
  char *type_str = type_to_string(meta->type, allocator);
  len += strlen(type_str);
  size_t offset = 0;
  char *str = allocator_alloc(allocator, len, NULL);
  strcpy(&str[offset], "func(");
  offset += 5;
  for (size_t idx = 0; idx < argc; idx++) {
    if (idx != 0) {
      strcpy(&str[offset], ", ");
      offset += 2;
    }
    strcpy(&str[offset], argv[idx]);
    offset += strlen(argv[idx]);
    allocator_free(allocator, argv[idx]);
  }
  str[offset++] = ')';
  str[offset++] = ':';
  str[offset++] = ' ';
  strcpy(&str[offset], type_str);
  offset += strlen(type_str);
  str[offset] = 0;
  allocator_free(allocator, type_str);
  return str;
}
static value_t function_call(value_t self, context_t ctx, size_t argc,
                             value_t argv[]) {
  return create_error(ctx, "not implement");
}

value_t create_function_type(context_t ctx, type_t type, size_t num_args,
                             type_t args[], bool variadic) {
  function_meta_t meta = create_function_meta(context_get_allocator(ctx), type,
                                              num_args, args, variadic);
  struct _type_operator_t opt = {
      .is_type_equal = function_is_equal,
      .type_to_string = function_type_to_string,
      .call = function_call,
  };
  return context_create_type(ctx, CUBEC_VALUE_TYPE_FUNCTION, sizeof(void *),
                             sizeof(void *), meta, &opt, NULL);
}
array_t function_type_get_arguments(type_t self) {
  function_meta_t meta = type_get_meta(self);
  return meta->arguments;
}
type_t function_type_get_type(type_t self) {
  function_meta_t meta = type_get_meta(self);
  return meta->type;
}
bool function_type_is_variadic(type_t self) {
  function_meta_t meta = type_get_meta(self);
  return meta->variadic;
}

value_t create_function(context_t ctx, type_t func_type, ast_node_t func,
                        bool mutable, const char *name) {
  return context_create_value(ctx, func_type, mutable, func, name);
}