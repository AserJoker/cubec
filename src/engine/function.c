#include "engine/function.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/location.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

typedef struct _function_meta_t *function_meta_t;
struct _function_meta_t {
  type_t type;
  array_t arguments;
  bool variadic;
};

static void function_meta_dispose(function_meta_t self, allocator_t allocator) {
  allocator_free(allocator, self->arguments);
}

static function_meta_t create_function_meta(allocator_t allocator, type_t type,
                                            size_t argc, argument_t argv[],
                                            bool variadic) {
  function_meta_t self =
      allocator_alloc(allocator, sizeof(struct _function_meta_t),
                      (dispose_fn_t)function_meta_dispose);
  self->type = type;
  array_initialize_t initialize = {
      .autofree = true,
  };
  self->arguments = create_array(allocator, &initialize);
  for (size_t idx = 0; idx < argc; idx++) {
    argument_t *arg =
        allocator_alloc(allocator, sizeof(struct _argument_t), NULL);
    arg->comptime = argv[idx].comptime;
    arg->type = argv[idx].type;
    arg->mut = argv[idx].mut;
    array_push(self->arguments, arg);
  }
  self->variadic = variadic;
  return self;
}

static value_t function_eq(value_t self, context_t ctx, value_t another) {
  type_t right_type = value_get_type(another);
  type_t left_type = value_get_type(self);
  if (strcmp(type_get_id(left_type), type_get_id(right_type)) != 0) {
    return create_error(ctx,
                        "invalid operands to binary expression ('%s' and '%s')",
                        type_get_name(left_type), type_get_name(right_type));
  }
  ast_node_t left_data = *(ast_node_t *)value_get_data(self);
  ast_node_t right_data = *(ast_node_t *)value_get_data(another);
  return create_comptime_bool(ctx, left_data == right_data, false, NULL);
}

static value_t function_ne(value_t self, context_t ctx, value_t another) {
  type_t right_type = value_get_type(another);
  type_t left_type = value_get_type(self);
  if (strcmp(type_get_id(left_type), type_get_id(right_type)) != 0) {
    return create_error(ctx,
                        "invalid operands to binary expression ('%s' and '%s')",
                        type_get_name(left_type), type_get_name(right_type));
  }
  ast_node_t left_data = *(ast_node_t *)value_get_data(self);
  ast_node_t right_data = *(ast_node_t *)value_get_data(another);
  return create_comptime_bool(ctx, left_data != right_data, false, NULL);
}
static value_t function_call(value_t self, context_t ctx, size_t argc,
                             value_t argv[]) {
  type_t type = value_get_type(self);
  function_meta_t meta = type_get_meta(type);
  for (size_t idx = 0; idx < argc; idx++) {
    if (idx >= array_get_size(meta->arguments)) {
      if (meta->variadic) {
        argument_t *arg_info =
            array_get(meta->arguments, array_get_size(meta->arguments) - 1);
        if (arg_info->comptime && !value_is_comptime(argv[idx])) {
          return create_error(ctx, "arguments %" PRIuPTR " is not comptime",
                              idx);
        }
        argv[idx] = value_safe_convert(argv[idx], ctx, arg_info->type);
        type_t type = value_get_type(argv[idx]);
        if (type_get_kind(type) == TYPE_KIND_ERROR) {
          return argv[idx];
        }
      } else {
        return create_error(
            ctx, "value requires %" PRIuPTR " arguments, receive %" PRIuPTR,
            array_get_size(meta->arguments), idx);
      }
    } else {
      argument_t *arg_info = array_get(meta->arguments, idx);
      if (arg_info->comptime && !value_is_comptime(argv[idx])) {
        return create_error(ctx, "arguments %" PRIuPTR " is not comptime", idx);
      }
      argv[idx] = value_safe_convert(argv[idx], ctx, arg_info->type);
      type_t type = value_get_type(argv[idx]);
      if (type_get_kind(type) == TYPE_KIND_ERROR) {
        return argv[idx];
      }
    }
  }
  ast_node_t node = *(ast_node_t *)value_get_data(self);
  ast_node_t kind = ast_get_child(node, "kind");
  if (location_is(kind->loc, "comptime")) {
    // TODO: resolve
  }
  return context_create_value(ctx, meta->type, NULL, false, false, NULL);
}
type_t create_function_type(context_t ctx, type_t type, size_t argc,
                            argument_t argv[], bool variadic) {
  allocator_t allocator = context_get_allocator(ctx);
  size_t len = 16;
  len += strlen(type_get_id(type));
  for (size_t idx = 0; idx < argc; idx++) {
    if (argv[idx].comptime) {
      len += strlen("comptime ");
    }
    if (!argv[idx].mut) {
      len += strlen("const ");
    }
    len += strlen(type_get_id(argv[idx].type));
  }
  char id[len + 1];
  size_t offset = 0;
  strcpy(&id[offset], "func(");
  offset += strlen("func(");
  for (size_t idx = 0; idx < argc; idx++) {
    if (idx != 0) {
      strcpy(&id[offset], ", ");
      offset += 2;
    }
    if (idx == argc - 1 && variadic) {
      strcpy(&id[offset], "...");
      offset += 3;
    }
    if (argv[idx].comptime) {
      strcpy(&id[offset], "comptime ");
      offset += strlen("comptime ");
    }
    if (!argv[idx].mut) {
      strcpy(&id[offset], "const ");
      offset += strlen("const ");
    }
    const char *type_id = type_get_id(argv[idx].type);
    strcpy(&id[offset], type_id);
    offset += strlen(type_id);
  }
  id[offset++] = ')';
  id[offset++] = ':';
  id[offset++] = ' ';
  const char *type_id = type_get_id(type);
  strcpy(&id[offset], type_id);
  offset += strlen(type_id);
  id[offset] = 0;
  type_t self = context_load_type(ctx, id);
  if (!self) {
    len = 16;
    len += strlen(type_get_name(type));
    for (size_t idx = 0; idx < argc; idx++) {
      if (argv[idx].comptime) {
        len += strlen("comptime ");
      }
      if (!argv[idx].mut) {
        len += strlen("const ");
      }
      len += strlen(type_get_id(argv[idx].type));
    }
    char name[len];
    offset = 0;
    strcpy(&name[offset], "func(");
    offset += strlen("func(");
    for (size_t idx = 0; idx < argc; idx++) {
      if (idx != 0) {
        strcpy(&name[offset], ", ");
        offset += 2;
      }
      if (idx == argc - 1 && variadic) {
        strcpy(&name[offset], "...");
        offset += 3;
      }
      if (argv[idx].comptime) {
        strcpy(&id[offset], "comptime ");
        offset += strlen("comptime ");
      }
      if (!argv[idx].mut) {
        strcpy(&id[offset], "const ");
        offset += strlen("const ");
      }
      const char *type_name = type_get_name(argv[idx].type);
      strcpy(&name[offset], type_name);
      offset += strlen(type_name);
    }
    name[offset++] = ')';
    name[offset++] = ':';
    name[offset++] = ' ';
    const char *type_id = type_get_id(type);
    strcpy(&name[offset], type_id);
    offset += strlen(type_id);
    name[offset] = 0;
    type_operator_t opt = {
        .eq = function_eq,
        .ne = function_ne,
        .call = function_call,
    };
    function_meta_t meta =
        create_function_meta(allocator, type, argc, argv, variadic);
    self = create_type(allocator, TYPE_KIND_FUNCTION, sizeof(void *),
                       sizeof(void *), name, id, &opt, meta);
    context_store_type(ctx, self);
  }
  return self;
}

value_t create_function(context_t ctx, type_t function_type, ast_node_t node) {
  return context_create_value(ctx, function_type, &node, false, true, NULL);
}

type_t function_type_get_type(type_t self) {
  function_meta_t meta = type_get_meta(self);
  return meta->type;
}

bool function_type_is_variadic(type_t self) {
  function_meta_t meta = type_get_meta(self);
  return meta->variadic;
}

array_t function_type_get_arguments(type_t self) {
  function_meta_t meta = type_get_meta(self);
  return meta->arguments;
}