#include "engine/function.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/hash_map.h"
#include "core/location.h"
#include "core/string.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/interrupt.h"
#include "engine/module.h"
#include "engine/scope.h"
#include "engine/str.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/function_body.h"
#include "resolve/type.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
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
  if (!type_is_equal(left_type, right_type)) {
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
  if (!type_is_equal(left_type, right_type)) {
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
  function_declar declar = *(function_declar *)value_get_data(self);
  ast_node_t arguments = ast_get_child(declar->node, "arguments");
  ast_node_t body = ast_get_child(declar->node, "body");
  ast_node_t return_type = ast_get_child(declar->node, "type");
  if (type_get_kind(type) == TYPE_KIND_COMPTIME_FUNCTION) {
    allocator_t allocator = context_get_allocator(ctx);
    type_t _global = declar->global;
    type_t _self = declar->bind;
    context_type_t current_type = context_get_type(ctx);
    context_set_type(ctx, CONTEXT_TYPE_FUNCTION);
    bool is_comptime = context_is_comptime(ctx);
    context_set_comptime(ctx, true);
    type_t current_global = context_get_global(ctx);
    context_set_global(ctx, _global);
    type_t current_self = context_get_self(ctx);
    context_set_self(ctx, _self);
    value_t current_function = context_get_function(ctx);
    context_set_function(ctx, self);
    scope_t current_scope = context_get_scope(ctx);
    scope_t scope = create_scope(allocator, context_get_root_scope(ctx));
    context_set_scope(ctx, scope);
    value_t result = NULL;
    hash_map_t closure = declar->closure;
    list_node_t it = hash_map_get_first(closure);
    while (it != hash_map_get_end(closure)) {
      const char *name = hash_map_node_get_key(it);
      value_t value = hash_map_node_get_value(it);
      value = value_clone(value, allocator);
      scope_store(scope, allocator, name, value);
      it = hash_map_node_get_next(it);
    }
    context_push_scope(ctx);
    scope = context_get_scope(ctx);
    for (size_t idx = 0; idx < ast_get_length(arguments); idx++) {
      ast_node_t arg_node = ast_get_item(arguments, idx);
      if (!value_is_comptime(argv[idx])) {
        result = create_comptime_error(
            ctx, arg_node, "argument %" PRIuPTR " is not comptime", idx);
        break;
      }
      ast_node_t identifier = ast_get_child(arg_node, "identifier");
      char *name = location_get(identifier->loc, allocator);
      if (scope_load(scope, name)) {
        result =
            create_comptime_error(ctx, arg_node, "redefinition of '%s'", name);
        allocator_free(allocator, name);
        break;
      }
      if (arg_node->type == NODE_TYPE_FUNCTION_ARGUMENT) {
        value_t val = value_clone(argv[idx], allocator);
        scope_store(scope, allocator, name, val);
      } else {
        // TODO: rest
      }
      allocator_free(allocator, name);
    }
    context_push_scope(ctx);
    if (!result) {
      value_t vreturn_type = resolve_type(ctx, return_type);
      type_t type = NULL;
      if (value_is_error(vreturn_type)) {
        result = vreturn_type;
      } else {
        type = *(type_t *)value_get_type(vreturn_type);
      }
      if (!result) {
        result = resolve_function_body(ctx, body);
        if (value_is_interrupt(result)) {
          result = interrupt_get_value(result);
        } else if (!value_is_error(result)) {
          result = value_safe_convert(result, ctx, type);
        }
      }
    }
    result = value_clone(result, allocator);
    context_set_scope(ctx, current_scope);
    allocator_free(allocator, scope);
    context_set_function(ctx, current_function);
    context_set_comptime(ctx, is_comptime);
    context_set_global(ctx, current_global);
    context_set_self(ctx, current_self);
    context_set_type(ctx, current_type);
    scope_store(current_scope, allocator, NULL, result);
    return result;
  }
  function_meta_t meta = type_get_meta(type);
  for (size_t idx = 0; idx < argc; idx++) {
    ast_node_t arg_node = ast_get_item(arguments, idx);
    if (idx >= array_get_size(meta->arguments)) {
      if (meta->variadic) {
        argument_t *arg_info =
            array_get(meta->arguments, array_get_size(meta->arguments) - 1);
        argv[idx] = value_safe_convert(argv[idx], ctx, arg_info->type);
        if (value_is_error(argv[idx])) {
          return argv[idx];
        }
      } else {
        return create_comptime_error(ctx, arg_node,
                                     "value requires %" PRIuPTR
                                     " arguments, receive %" PRIuPTR,
                                     array_get_size(meta->arguments), argc);
      }
    } else {
      argument_t *arg_info = array_get(meta->arguments, idx);
      argv[idx] = value_safe_convert(argv[idx], ctx, arg_info->type);
      type_t type = value_get_type(argv[idx]);
      if (type_get_kind(type) == TYPE_KIND_ERROR) {
        return argv[idx];
      }
    }
  }
  if (context_is_comptime(ctx)) {
    function_declar declar = *(function_declar *)value_get_data(self);
    allocator_t allocator = context_get_allocator(ctx);
    type_t _global = declar->global;
    type_t _self = declar->bind;
    context_type_t current_type = context_get_type(ctx);
    context_set_type(ctx, CONTEXT_TYPE_FUNCTION);
    bool is_comptime = context_is_comptime(ctx);
    context_set_comptime(ctx, true);
    type_t current_global = context_get_global(ctx);
    context_set_global(ctx, _global);
    type_t current_self = context_get_self(ctx);
    context_set_self(ctx, _self);
    value_t current_function = context_get_function(ctx);
    context_set_function(ctx, self);
    scope_t current_scope = context_get_scope(ctx);
    scope_t scope = create_scope(allocator, context_get_root_scope(ctx));
    context_set_scope(ctx, scope);
    value_t result = NULL;
    ast_node_t arguments = ast_get_child(declar->node, "arguments");
    ast_node_t body = ast_get_child(declar->node, "body");
    ast_node_t return_type = ast_get_child(declar->node, "type");
    hash_map_t closure = declar->closure;
    list_node_t it = hash_map_get_first(closure);
    while (it != hash_map_get_end(closure)) {
      const char *name = hash_map_node_get_key(it);
      value_t value = hash_map_node_get_value(it);
      value = value_clone(value, allocator);
      scope_store(scope, allocator, name, value);
      it = hash_map_node_get_next(it);
    }
    context_push_scope(ctx);
    scope = context_get_scope(ctx);
    for (size_t idx = 0; idx < ast_get_length(arguments); idx++) {
      ast_node_t arg_node = ast_get_item(arguments, idx);
      if (!value_is_comptime(argv[idx])) {
        result = create_comptime_error(
            ctx, arg_node, "argument %" PRIuPTR " is not comptime", idx);
        break;
      }
      ast_node_t identifier = ast_get_child(arg_node, "identifier");
      char *name = location_get(identifier->loc, allocator);
      if (scope_load(scope, name)) {
        result =
            create_comptime_error(ctx, arg_node, "redefinition of '%s'", name);
        allocator_free(allocator, name);
        break;
      }
      if (arg_node->type == NODE_TYPE_FUNCTION_ARGUMENT) {
        value_t val = value_clone(argv[idx], allocator);
        scope_store(scope, allocator, name, val);
      } else {
        // TODO: rest
      }
      allocator_free(allocator, name);
    }
    context_push_scope(ctx);
    if (!result) {
      value_t vreturn_type = resolve_type(ctx, return_type);
      type_t type = NULL;
      if (value_is_error(vreturn_type)) {
        result = vreturn_type;
      } else {
        type = *(type_t *)value_get_type(vreturn_type);
      }
      if (!result) {
        result = resolve_function_body(ctx, body);
        if (value_is_interrupt(result)) {
          result = interrupt_get_value(result);
        } else if (!value_is_error(result)) {
          result = value_safe_convert(result, ctx, type);
        }
      }
    }
    result = value_clone(result, allocator);
    context_set_scope(ctx, current_scope);
    allocator_free(allocator, scope);
    context_set_function(ctx, current_function);
    context_set_comptime(ctx, is_comptime);
    context_set_global(ctx, current_global);
    context_set_self(ctx, current_self);
    context_set_type(ctx, current_type);
    scope_store(current_scope, allocator, NULL, result);
    return result;
  }
  return context_create_value(ctx, meta->type, NULL, false, false, NULL);
}

static bool function_type_is_equal(type_t self, type_t another) {
  function_meta_t src_meta = type_get_meta(self);
  function_meta_t dst_meta = type_get_meta(another);
  if (!type_is_equal(src_meta->type, dst_meta->type)) {
    return false;
  }
  if (src_meta->variadic != dst_meta->variadic) {
    return false;
  }
  if (array_get_size(src_meta->arguments) !=
      array_get_size(dst_meta->arguments)) {
    return false;
  }
  for (size_t idx = 0; idx < array_get_size(src_meta->arguments); idx++) {
    argument_t *src_arg = array_get(src_meta->arguments, idx);
    argument_t *dst_arg = array_get(dst_meta->arguments, idx);
    if (src_arg->mut != dst_arg->mut) {
      return false;
    }
    if (!type_is_equal(src_arg->type, dst_arg->type)) {
      return false;
    }
  }
  return true;
}

type_t create_function_type(context_t ctx, type_t type, size_t argc,
                            argument_t argv[], bool variadic) {
  allocator_t allocator = context_get_allocator(ctx);
  size_t len = 16;
  len += strlen(type_get_id(type));
  for (size_t idx = 0; idx < argc; idx++) {
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
        .type_eq = function_type_is_equal,
        .assigment = value_default_assigment,
    };
    function_meta_t meta =
        create_function_meta(allocator, type, argc, argv, variadic);
    self = create_type(allocator, TYPE_KIND_FUNCTION, sizeof(void *),
                       sizeof(void *), name, id, &opt, meta);
    context_store_type(ctx, self);
  }
  return self;
}

void function_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  type_operator_t opt = {
      .eq = function_eq,
      .ne = function_ne,
      .call = function_call,
      .type_eq = type_default_eq,
      .assigment = value_default_assigment,
  };
  type_t comptime_func = create_type(
      allocator, TYPE_KIND_COMPTIME_FUNCTION, sizeof(function_declar),
      sizeof(function_declar), "comptime_func", "comptime_func", &opt, NULL);
  context_store_type(ctx, comptime_func);
}

value_t create_function(context_t ctx, type_t function_type, ast_node_t node) {
  allocator_t allocator = context_get_allocator(ctx);
  module_t module = context_get_module(ctx);
  const char *parent_name = NULL;
  value_t current_function = context_get_function(ctx);
  if (current_function) {
    value_t function_name = function_get_id(ctx, current_function);
    parent_name = *(const char **)value_get_data(function_name);
  } else {
    type_t self = context_get_self(ctx);
    parent_name = type_get_id(self);
  }
  ast_node_t identifier = ast_get_child(node, "identifier");
  char *id_str = NULL;
  if (identifier) {
    id_str = location_get(identifier->loc, allocator);
  }
  const char *func_name = id_str;
  if (!func_name) {
    func_name = "function";
  }
  size_t len = strlen(parent_name) + strlen(func_name) + 2;
  char base_fullname[len + 1];
  sprintf(base_fullname, "%s_%s", parent_name, func_name);
  allocator_free(allocator, id_str);
  char *id = NULL;
  if (module_get_function(module, base_fullname)) {
    for (size_t idx = 0;; idx++) {
      size_t len = snprintf(NULL, 0, "%s_%" PRIuPTR, base_fullname, idx);
      char fullname[len + 1];
      sprintf(fullname, "%s_%" PRIuPTR, base_fullname, idx);
      if (!module_get_function(module, fullname)) {
        id = create_cstring(allocator, fullname);
        break;
      }
    }
  } else {
    id = create_cstring(allocator, base_fullname);
  }
  function_declar declar = context_store_function_declar(ctx, node, id);
  value_t func = create_value(allocator, function_type, false, &declar, true);
  allocator_free(allocator, id);
  module_add_function(module, func);
  return func;
}
value_t create_comptime_function(context_t ctx, ast_node_t node) {
  allocator_t allocator = context_get_allocator(ctx);
  module_t module = context_get_module(ctx);
  const char *parent_name = NULL;
  value_t current_function = context_get_function(ctx);
  if (current_function) {
    value_t function_name = function_get_id(ctx, current_function);
    parent_name = *(const char **)value_get_data(function_name);
  } else {
    type_t self = context_get_self(ctx);
    parent_name = type_get_id(self);
  }
  ast_node_t identifier = ast_get_child(node, "identifier");
  char *id_str = NULL;
  if (identifier) {
    id_str = location_get(identifier->loc, allocator);
  }
  const char *func_name = id_str;
  if (!func_name) {
    func_name = "function";
  }
  size_t len = strlen(parent_name) + strlen(func_name) + 2;
  char base_fullname[len + 1];
  sprintf(base_fullname, "%s_%s", parent_name, func_name);
  allocator_free(allocator, id_str);
  char *id = NULL;
  if (module_get_function(module, base_fullname)) {
    for (size_t idx = 0;; idx++) {
      size_t len = snprintf(NULL, 0, "%s_%" PRIuPTR, base_fullname, idx);
      char fullname[len + 1];
      sprintf(fullname, "%s_%" PRIuPTR, base_fullname, idx);
      if (!module_get_function(module, fullname)) {
        id = create_cstring(allocator, fullname);
        break;
      }
    }
  } else {
    id = create_cstring(allocator, base_fullname);
  }
  type_t type = context_load_type(ctx, "comptime_func");
  function_declar declar = context_store_function_declar(ctx, node, id);
  value_t func = create_value(allocator, type, false, &declar, true);
  allocator_free(allocator, id);
  module_add_function(module, func);
  return func;
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
value_t function_get_id(context_t ctx, value_t self) {
  function_declar declar = *(function_declar *)value_get_data(self);
  return create_str(ctx, declar->id);
}
value_t function_add_closure(context_t ctx, value_t self, const char *name,
                             value_t value) {
  function_declar declar = *(function_declar *)value_get_data(self);
  if (hash_map_has(declar->closure, name, NULL, NULL)) {
    return create_error(ctx, "redefinition of '%s'", name);
  }
  allocator_t allocator = context_get_allocator(ctx);
  hash_map_set(declar->closure, create_cstring(allocator, name),
               value_clone(value, allocator), NULL, NULL);
  return context_get_undefined(ctx);
}