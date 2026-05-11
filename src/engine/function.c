#include "engine/function.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/location.h"
#include "core/string.h"
#include "engine/array.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/interrupt.h"
#include "engine/module.h"
#include "engine/scope.h"
#include "engine/slice.h"
#include "engine/str.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include "resolve/function_body.h"
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
                                            array_t argv, bool variadic) {
  function_meta_t self =
      allocator_alloc(allocator, sizeof(struct _function_meta_t),
                      (dispose_fn_t)function_meta_dispose);
  self->type = type;
  self->arguments = argv;
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
  function_declar_t left_data = *(function_declar_t *)value_get_data(self);
  function_declar_t right_data = *(function_declar_t *)value_get_data(another);
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

static value_t resolve_closure(context_t ctx, array_t closure) {
  allocator_t allocator = context_get_allocator(ctx);
  for (size_t idx = 0; idx < array_get_size(closure); idx++) {
    closure_item_t item = array_get(closure, idx);
    value_t value = value_clone(item->value, allocator);
    value_t err = context_declar(ctx, item->name, value);
    if (value_is_error(err)) {
      return err;
    }
  }
  return context_get_undefined(ctx);
}
static value_t resolve_result(context_t ctx, scope_t current_scope,
                              scope_t scope, value_t value) {
  allocator_t allocator = context_get_allocator(ctx);
  value = value_clone(value, allocator);
  scope_store(current_scope, allocator, NULL, value);
  context_set_scope(ctx, current_scope);
  allocator_free(allocator, scope);
  return value;
}

static value_t resolve_rest(context_t ctx, type_t type, size_t offset,
                            size_t argc, value_t argv[], bool mut) {
  type_t base_type = slice_type_get_type(type);
  size_t len = argc - offset;
  type_t array_type = create_array_type(ctx, base_type, len);
  if (context_is_comptime(ctx)) {
    uint8_t array_data[type_get_size(array_type)];
    value_t arr =
        context_create_value(ctx, array_type, array_data, true, true, NULL);
    for (size_t i = 0; offset + i < argc; i++) {
      value_t arg = argv[offset + i];
      if (!value_is_comptime(arg)) {
        return create_error(ctx, "argument %" PRIuPTR " is not comptime",
                            offset);
      }
      value_t key = create_comptime_u64(ctx, i, false, NULL);
      value_t err = value_set(arr, ctx, key, arg);
      if (value_is_error(err)) {
        return err;
      }
    }
    void *base_data = value_get_data(arr);
    return create_comptime_slice(ctx, type, base_data, 0, len, mut);
  } else {
    value_t arr =
        context_create_value(ctx, array_type, NULL, true, false, NULL);
    for (size_t i = 0; offset + i < argc; i++) {
      value_t key = create_comptime_u64(ctx, i, false, NULL);
      value_t err = value_set(arr, ctx, key, argv[offset + i]);
      if (value_is_error(err)) {
        return err;
      }
    }
    return context_create_value(ctx, type, NULL, false, false, NULL);
  }
}

static value_t function_call(value_t self, context_t ctx, size_t argc,
                             value_t argv[]) {
  allocator_t allocator = context_get_allocator(ctx);
  type_t type = value_get_type(self);
  type_t function_type = value_get_type(self);
  value_t function = self;
  function_declar_t declar = *(function_declar_t *)value_get_data(self);
  ast_node_t arguments = ast_get_child(declar->node, "arguments");
  ast_node_t body = ast_get_child(declar->node, "body");

  context_frame_t frame = context_push(ctx, self, CONTEXT_TYPE_FUNCTION,
                                       declar->global, declar->bind);
  bool is_comptime = context_is_comptime(ctx);
  scope_t current_scope = context_get_scope(ctx);
  scope_t scope = create_scope(allocator, context_get_root_scope(ctx));
  context_set_scope(ctx, scope);
  value_t result = NULL;
  value_t err = resolve_closure(ctx, declar->closure);
  if (value_is_error(err)) {
    result = err;
    goto onfinish;
  }
  context_push_scope(ctx);
  function_meta_t meta = type_get_meta(function_type);
  size_t size = array_get_size(meta->arguments);
  if (meta->variadic) {
    size--;
  }
  if (argc < size || (argc > size && !meta->variadic)) {
    result = create_error(
        ctx, "function requires %" PRIuPTR " arguments, received %" PRIuPTR,
        size, argc);
    goto onfinish;
  }
  for (size_t idx = 0; idx < array_get_size(meta->arguments); idx++) {
    ast_node_t arg = ast_get_item(arguments, idx);
    value_t value = NULL;
    argument_t info = array_get(meta->arguments, idx);
    ast_node_t identifier = ast_get_child(arg, "identifier");

    if (arg->type == NODE_TYPE_FUNCTION_ARGUMENT_REST) {
      if (!info->type) {
        break;
      }
      if (type_get_kind(info->type) != TYPE_KIND_SLICE) {
        result =
            create_error(ctx, "rest argument %" PRIuPTR " is not slice", idx);
        goto onfinish;
      }
      value = resolve_rest(ctx, info->type, idx, argc, argv, info->mut);
      if (value_is_error(value)) {
        result = value;
        goto onfinish;
      }
    } else {
      if (context_is_comptime(ctx)) {
        if (!value_is_comptime(argv[idx])) {
          result =
              create_error(ctx, "argument %" PRIuPTR " is not comptime", idx);
          goto onfinish;
        }
      } else {
        if (type_get_kind(info->type) == TYPE_KIND_TYPE) {
          result =
              create_error(ctx, "type value only declared in comptime context");
          goto onfinish;
        }
      }
      value = argv[idx];
      value = value_safe_convert(value, ctx, info->type);
      if (value_is_error(value)) {
        result = value;
        goto onfinish;
      }
    }
    if (type_get_kind(info->type) == TYPE_KIND_PTR ||
        type_get_kind(info->type) == TYPE_KIND_PARRAY ||
        type_get_kind(info->type) == TYPE_KIND_OPAQUE ||
        type_get_kind(info->type) == TYPE_KIND_SLICE) {
      if (info->mut && !value_is_mut(value)) {
        result =
            create_error(ctx, "cannot initialize '%s' with 'const %s'",
                         type_get_name(info->type), type_get_name(info->type));
        goto onfinish;
      }
    }
    char *name = location_get(identifier->loc, allocator);
    void *data = value_get_data(value);
    bool cmptime = value_is_comptime(value);
    value_t err =
        context_create_value(ctx, info->type, data, info->mut, cmptime, name);
    allocator_free(allocator, name);
    if (value_is_error(err)) {
      result = err;
      goto onfinish;
    }
  }
  context_push_scope(ctx);
  if (context_is_comptime(ctx)) {
    if (!body) {
      result = create_error(ctx, "cannot call extern function in comptime");
      goto onfinish;
    }
    context_set_comptime(ctx, true);
    result = resolve_function_body(ctx, body);
  } else {
    function_meta_t meta = type_get_meta(function_type);
    result = context_create_value(ctx, meta->type, NULL, false, false, NULL);
  }
  if (value_is_error(result)) {
    goto onfinish;
  }
  if (value_is_interrupt(result)) {
    result = interrupt_get_value(result);
  }
onfinish:
  result = resolve_result(ctx, current_scope, scope, result);
  context_pop(ctx, frame);
  context_set_comptime(ctx, is_comptime);
  return result;
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
    argument_t src_arg = array_get(src_meta->arguments, idx);
    argument_t dst_arg = array_get(dst_meta->arguments, idx);
    if (src_arg->mut != dst_arg->mut) {
      return false;
    }
    if (!type_is_equal(src_arg->type, dst_arg->type)) {
      return false;
    }
  }
  return true;
}

type_t create_function_type(context_t ctx, type_t type, array_t argv,
                            bool variadic) {
  size_t argc = array_get_size(argv);
  allocator_t allocator = context_get_allocator(ctx);
  string_t sid = create_string(allocator, NULL);
  string_concat(sid, allocator, "F");
  const char *type_id = type_get_id(type);
  string_concat(sid, allocator, type_id);
  for (size_t idx = 0; idx < argc; idx++) {
    argument_t arg = array_get(argv, idx);
    if (idx != 0) {
      string_concat(sid, allocator, "A");
    }
    if (!arg->mut) {
      string_concat(sid, allocator, "C");
    }
    if (idx == argc - 1 && variadic) {
      string_concat(sid, allocator, "V");
    }
    if (arg->type) {
      const char *type_id = type_get_id(arg->type);
      string_concat(sid, allocator, type_id);
    }
  }
  const char *id = string_get(sid);
  type_t self = context_load_type(ctx, id);
  if (!self) {
    string_t sname = create_string(allocator, NULL);
    string_concat(sname, allocator, "func(");
    for (size_t idx = 0; idx < argc; idx++) {
      argument_t arg = array_get(argv, idx);
      if (idx != 0) {
        string_concat(sname, allocator, ", ");
      }
      if (!arg->mut) {
        string_concat(sname, allocator, "const ");
      }
      if (variadic && idx == argc - 1) {
        string_concat(sname, allocator, "...");
      }
      if (arg->type) {
        const char *type_name = type_get_name(arg->type);
        string_concat(sname, allocator, type_name);
      }
    }
    string_concat(sname, allocator, "): ");
    const char *type_name = type_get_name(type);
    string_concat(sname, allocator, type_name);
    const char *name = string_get(sname);
    type_operator_t opt = {
        .eq = function_eq,
        .ne = function_ne,
        .call = function_call,
        .type_eq = function_type_is_equal,
        .assigment = value_default_assigment,
    };
    function_meta_t meta =
        create_function_meta(allocator, type, argv, variadic);
    self = create_type(allocator, TYPE_KIND_FUNCTION, sizeof(void *),
                       sizeof(void *), name, id, &opt, meta);
    context_store_type(ctx, self);
    allocator_free(allocator, sname);
  } else {
    allocator_free(allocator, argv);
  }
  allocator_free(allocator, sid);
  return self;
}

void function_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  type_operator_t comptime_opt = {
      .eq = function_eq,
      .ne = function_ne,
      .type_eq = type_default_eq,
      .assigment = value_default_assigment,
  };
  type_t comptime_func =
      create_type(allocator, TYPE_KIND_COMPTIME_FUNCTION,
                  sizeof(function_declar_t), sizeof(function_declar_t),
                  "comptime_func", "comptime_func", &comptime_opt, NULL);
  context_store_type(ctx, comptime_func);
  type_t template_func = create_type(
      allocator, TYPE_KIND_TEMPLATE_FUNCTION, sizeof(function_declar_t),
      sizeof(function_declar_t), "template_func", "template_func", NULL, NULL);
  context_store_type(ctx, template_func);
}

value_t create_function(context_t ctx, type_t function_type, ast_node_t node,
                        array_t closure) {
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
    func_name = "func";
  }
  size_t len = strlen(parent_name) + strlen(func_name) + 2;
  char base_fullname[len + 1];
  sprintf(base_fullname, "%s_%s", parent_name, func_name);
  allocator_free(allocator, id_str);
  char *id = module_generator_func_id(module, allocator, base_fullname);
  type_t global = context_get_global(ctx);
  type_t self = context_get_self(ctx);
  function_declar_t declar = create_function_declar(
      allocator, FUNC_TYPE_AST, global, self, id, node, closure);
  allocator_free(allocator, id);
  context_store_function_declar(ctx, declar);
  value_t func = create_value(allocator, function_type, false, &declar, true);
  module_add_function(module, func);
  return func;
}
value_t create_comptime_function(context_t ctx, ast_node_t node,
                                 array_t closure) {
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
  type_t type = context_load_type(ctx, "comptime_func");
  char *id = module_generator_func_id(module, allocator, base_fullname);
  type_t global = context_get_global(ctx);
  type_t self = context_get_self(ctx);
  function_declar_t declar = create_function_declar(
      allocator, FUNC_TYPE_AST, global, self, id, node, closure);
  allocator_free(allocator, id);
  context_store_function_declar(ctx, declar);
  return context_create_value(ctx, type, &declar, false, true, NULL);
}
value_t create_template_function(context_t ctx, ast_node_t node,
                                 array_t closure) {
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
    func_name = "template";
  }
  size_t len = strlen(parent_name) + strlen(func_name) + 2;
  char base_fullname[len + 1];
  sprintf(base_fullname, "%s_%s", parent_name, func_name);
  allocator_free(allocator, id_str);
  type_t type = context_load_type(ctx, "template_func");
  char *id = module_generator_func_id(module, allocator, base_fullname);
  type_t global = context_get_global(ctx);
  type_t self = context_get_self(ctx);
  function_declar_t declar = create_function_declar(
      allocator, FUNC_TYPE_AST, global, self, id, node, closure);
  allocator_free(allocator, id);
  context_store_function_declar(ctx, declar);
  return context_create_value(ctx, type, &declar, false, true, NULL);
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
  function_declar_t declar = *(function_declar_t *)value_get_data(self);
  return create_str(ctx, declar->id);
}
static void closure_item_dispose(closure_item_t self, allocator_t allocator) {
  allocator_free(allocator, self->name);
  allocator_free(allocator, self->value);
}
closure_item_t create_closure_item(allocator_t allocaotr, const char *name,
                                   value_t value) {
  closure_item_t self =
      allocator_alloc(allocaotr, sizeof(struct _closure_item_t),
                      (dispose_fn_t)closure_item_dispose);
  self->name = create_cstring(allocaotr, name);
  self->value = value_clone(value, allocaotr);
  return self;
}

static void function_declar_dispsoe(function_declar_t self,
                                    allocator_t allocator) {
  allocator_free(allocator, self->id);
  allocator_free(allocator, self->closure);
}

function_declar_t create_function_declar(allocator_t allocator,
                                         func_type_t type, type_t global,
                                         type_t self, const char *id,
                                         void *data, array_t closure) {
  function_declar_t declar =
      allocator_alloc(allocator, sizeof(struct _function_declar_t),
                      (dispose_fn_t)function_declar_dispsoe);
  declar->bind = self;
  declar->global = global;
  declar->closure = closure;
  declar->id = create_cstring(allocator, id);
  declar->data = data;
  declar->type = type;
  return declar;
}