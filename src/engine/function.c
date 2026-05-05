#include "engine/function.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/hash_map.h"
#include "core/location.h"
#include "core/string.h"
#include "engine/array.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/interrupt.h"
#include "engine/module.h"
#include "engine/ptr.h"
#include "engine/scope.h"
#include "engine/slice.h"
#include "engine/str.h"
#include "engine/type.h"
#include "engine/unsigned.h"
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
  allocator_t allocator = context_get_allocator(ctx);
  type_t function_type = value_get_type(self);
  value_t function = self;
  bool is_runtime = !context_is_comptime(ctx) &&
                    (type_get_kind(function_type) == TYPE_KIND_FUNCTION ||
                     type_get_kind(function_type) == TYPE_KIND_TEMPLATE);
  function_declar declar = *(function_declar *)value_get_data(self);
  ast_node_t arguments = ast_get_child(declar->node, "arguments");
  ast_node_t body = ast_get_child(declar->node, "body");
  ast_node_t return_type = ast_get_child(declar->node, "type");
  ast_node_t kind = ast_get_child(declar->node, "kind");

  value_t current_function = context_set_function(ctx, self);
  context_type_t current_type = context_set_type(ctx, CONTEXT_TYPE_FUNCTION);
  type_t current_global = context_set_global(ctx, declar->global);
  type_t current_self = context_set_self(ctx, declar->bind);
  scope_t current_scope = context_get_scope(ctx);
  scope_t scope = create_scope(allocator, current_scope);
  bool is_comptime = context_is_comptime(ctx);
  context_set_scope(ctx, scope);
  value_t result = NULL;
  list_node_t it = hash_map_get_first(declar->closure);
  while (it != hash_map_get_end(declar->closure)) {
    const char *name = hash_map_node_get_key(it);
    value_t value = hash_map_node_get_value(it);
    value = value_clone(value, allocator);
    value_t err = context_declar(ctx, name, value);
    if (value_is_error(err)) {
      result = err;
      goto onfinish;
    }
    it = hash_map_node_get_next(it);
  }
  context_push_scope(ctx);
  if (type_get_kind(function_type) == TYPE_KIND_TEMPLATE) {
    size_t size = 0;
    bool variadic = false;
    for (size_t idx = 0; idx < ast_get_length(arguments); idx++) {
      ast_node_t arg = ast_get_item(arguments, idx);
      if (arg->type == NODE_TYPE_FUNCTION_ARGUMENT_REST) {
        variadic = true;
        break;
      }
      size++;
    }
    if (argc < size || (argc > size && !variadic)) {
      result = create_error(
          ctx, "function requires %" PRIuPTR " arguments, received %" PRIuPTR,
          size, argc);
      goto onfinish;
    }
    size_t argument_count = ast_get_length(arguments);
    argument_t args[argument_count];
    for (size_t idx = 0; idx < argument_count; idx++) {
      ast_node_t arg = ast_get_item(arguments, idx);
      ast_node_t identifier = ast_get_child(arg, "identifier");
      ast_node_t mut = ast_get_child(arg, "mut");
      ast_node_t type = ast_get_child(arg, "type");
      args[idx].mut = mut == NULL;
      type_t t = NULL;
      if (location_is(type->loc, "infer")) {
        if (arg->type == NODE_TYPE_FUNCTION_ARGUMENT_REST) {
          if (idx >= argc) {
            t = create_slice_type(ctx, context_load_type(ctx, "void"));
          } else {
            t = create_slice_type(ctx, value_get_type(argv[idx]));
          }
        } else {
          t = value_get_type(argv[idx]);
        }
      } else {
        value_t vt = resolve_type(ctx, type);
        if (value_is_error(vt)) {
          result = vt;
          goto onfinish;
        }
        t = *(type_t *)value_get_data(vt);
      }
      args[idx].type = t;
      value_t value = NULL;
      if (arg->type == NODE_TYPE_FUNCTION_ARGUMENT_REST) {
        if (type_get_kind(t) != TYPE_KIND_SLICE) {
          result =
              create_error(ctx, "rest argument %" PRIuPTR " is not slice", idx);
          goto onfinish;
        }
        type_t base_type = slice_type_get_type(t);
        size_t len = argc - idx;
        type_t array_type = create_array_type(ctx, base_type, len);
        if (context_is_comptime(ctx)) {
          uint8_t array_data[type_get_size(array_type)];
          value_t arr = context_create_value(ctx, array_type, array_data, false,
                                             true, NULL);
          for (size_t i = idx; idx + i < argc; i++) {
            value_t err =
                value_set(arr, ctx, create_comptime_u64(ctx, i, false, NULL),
                          argv[idx + i]);
            if (value_is_error(err)) {
              result = err;
              goto onfinish;
            }
          }
          void *base_data = value_get_data(arr);
          value = create_comptime_slice(ctx, t, base_data, 0, len);
        } else {
          value_t arr =
              context_create_value(ctx, array_type, NULL, true, false, NULL);
          for (size_t i = idx; idx + i < argc; i++) {
            value_t err =
                value_set(arr, ctx, create_comptime_u64(ctx, i, false, NULL),
                          argv[idx + i]);
            if (value_is_error(err)) {
              result = err;
              goto onfinish;
            }
          }
          value = context_create_value(ctx, t, NULL, false, false, NULL);
        }
      } else {
        type_t vt = value_get_type(argv[idx]);
        value = argv[idx];
        if (type_get_kind(vt) == TYPE_KIND_STR) {
          if (!args[idx].mut && type_get_kind(t) == TYPE_KIND_PARRAY) {
            type_t base_type = ptr_type_get_type(t);
            if (type_get_kind(base_type) == TYPE_KIND_UNSIGNED &&
                type_get_size(base_type) == sizeof(uint8_t)) {
              const char *src = *(const char **)value_get_data(value);
              value = context_create_value(ctx, t, &src, false, true, NULL);
            }
          }
        }
        value = value_safe_convert(value, ctx, t);
        if (value_is_error(value)) {
          result = value;
          goto onfinish;
        }
      }
      char *name = location_get(identifier->loc, allocator);
      void *data = value_get_data(value);
      value_t err =
          context_create_value(ctx, args[idx].type, data, args[idx].mut,
                               value_is_comptime(value), name);
      allocator_free(allocator, name);
      if (value_is_error(err)) {
        result = err;
        goto onfinish;
      }
    }
    value_t vrtype = resolve_type(ctx, return_type);
    if (value_is_error(vrtype)) {
      result = vrtype;
      goto onfinish;
    }
    type_t rtype = *(type_t *)value_get_data(vrtype);
    function_type =
        create_function_type(ctx, rtype, argument_count, args, variadic);
    function =
        context_create_value(ctx, function_type, &declar, false, true, NULL);
    context_set_function(ctx, function);
    context_push_scope(ctx);
    if (is_runtime) {
      context_set_comptime(ctx, false);
      value_t err = resolve_function_body(ctx, body);
      if (value_is_error(err)) {
        result = err;
        goto onfinish;
      }
    }
  } else if (type_get_kind(function_type) == TYPE_KIND_COMPTIME_FUNCTION) {
    size_t size = 0;
    bool variadic = false;
    for (size_t idx = 0; idx < ast_get_length(arguments); idx++) {
      ast_node_t arg = ast_get_item(arguments, idx);
      if (arg->type == NODE_TYPE_FUNCTION_ARGUMENT_REST) {
        variadic = true;
        break;
      }
      size++;
    }
    if (argc < size || (argc > size && !variadic)) {
      result = create_error(
          ctx, "function requires %" PRIuPTR " arguments, received %" PRIuPTR,
          size, argc);
      goto onfinish;
    }
    size_t argument_count = ast_get_length(arguments);
    argument_t args[argument_count];
    for (size_t idx = 0; idx < argument_count; idx++) {
      ast_node_t arg = ast_get_item(arguments, idx);
      ast_node_t identifier = ast_get_child(arg, "identifier");
      ast_node_t mut = ast_get_child(arg, "mut");
      ast_node_t type = ast_get_child(arg, "type");
      args[idx].mut = mut == NULL;
      type_t t = NULL;
      if (location_is(type->loc, "infer")) {
        if (idx >= argc) {
          t = create_slice_type(ctx, context_load_type(ctx, "void"));
        } else {
          t = create_slice_type(ctx, value_get_type(argv[idx]));
        }
      } else {
        value_t vt = resolve_type(ctx, type);
        if (value_is_error(vt)) {
          result = vt;
          goto onfinish;
        }
        t = *(type_t *)value_get_data(vt);
      }
      args[idx].type = t;
      value_t value = NULL;
      if (arg->type == NODE_TYPE_FUNCTION_ARGUMENT_REST) {
        if (type_get_kind(t) != TYPE_KIND_SLICE) {
          result =
              create_error(ctx, "rest argument %" PRIuPTR " is not slice", idx);
          goto onfinish;
        }
        type_t base_type = slice_type_get_type(t);
        size_t len = argc - idx;
        type_t array_type = create_array_type(ctx, base_type, len);
        uint8_t array_data[type_get_size(array_type)];
        value_t arr = context_create_value(ctx, array_type, array_data, false,
                                           true, NULL);
        for (size_t i = idx; idx + i < argc; i++) {
          value_t err =
              value_set(arr, ctx, create_comptime_u64(ctx, i, false, NULL),
                        argv[idx + i]);
          if (value_is_error(err)) {
            result = err;
            goto onfinish;
          }
        }
        void *base_data = value_get_data(arr);
        value = create_comptime_slice(ctx, t, base_data, 0, len);
      } else {
        value = argv[idx];
        if (!value_is_comptime(value)) {
          result =
              create_error(ctx, "argument %" PRIuPTR " is not comptime", idx);
          goto onfinish;
        }
        type_t vt = value_get_type(value);
        if (type_get_kind(vt) == TYPE_KIND_STR) {
          if (!args[idx].mut && type_get_kind(t) == TYPE_KIND_PARRAY) {
            type_t base_type = ptr_type_get_type(t);
            if (type_get_kind(base_type) == TYPE_KIND_UNSIGNED &&
                type_get_size(base_type) == sizeof(uint8_t)) {
              const char *src = *(const char **)value_get_data(value);
              value = context_create_value(ctx, t, &src, false, true, NULL);
            }
          }
        }
        value = value_safe_convert(value, ctx, t);
        if (value_is_error(value)) {
          result = value;
          goto onfinish;
        }
      }
      char *name = location_get(identifier->loc, allocator);
      void *data = value_get_data(value);
      value_t err = context_create_value(ctx, args[idx].type, data,
                                         args[idx].mut, true, name);
      allocator_free(allocator, name);
      if (value_is_error(err)) {
        result = err;
        goto onfinish;
      }
    }
    value_t vrtype = resolve_type(ctx, return_type);
    if (value_is_error(vrtype)) {
      result = vrtype;
      goto onfinish;
    }
    type_t rtype = *(type_t *)value_get_data(vrtype);
    function_type =
        create_function_type(ctx, rtype, argument_count, args, variadic);
    function =
        context_create_value(ctx, function_type, &declar, false, true, NULL);
    context_set_function(ctx, function);
    context_push_scope(ctx);
  } else {
    function_meta_t meta = type_get_meta(function_type);
    size_t size = array_get_size(meta->arguments);
    if (argc < size || (argc > size && !meta->variadic)) {
      result = create_error(
          ctx, "function requires %" PRIuPTR " arguments, received %" PRIuPTR,
          size, argc);
      goto onfinish;
    }
    for (size_t idx = 0; idx < array_get_size(meta->arguments); idx++) {
      ast_node_t arg = ast_get_item(arguments, idx);
      value_t value = NULL;
      argument_t *info = array_get(meta->arguments, idx);
      ast_node_t identifier = ast_get_child(arg, "identifier");
      if (arg->type == NODE_TYPE_FUNCTION_ARGUMENT_REST) {
        if (type_get_kind(info->type) != TYPE_KIND_SLICE) {
          result =
              create_error(ctx, "rest argument %" PRIuPTR " is not slice", idx);
          goto onfinish;
        }
        type_t base_type = slice_type_get_type(info->type);
        size_t len = argc - idx;
        type_t array_type = create_array_type(ctx, base_type, len);
        if (context_is_comptime(ctx)) {
          uint8_t array_data[type_get_size(array_type)];
          value_t arr = context_create_value(ctx, array_type, array_data, false,
                                             true, NULL);
          for (size_t i = idx; idx + i < argc; i++) {
            value_t err =
                value_set(arr, ctx, create_comptime_u64(ctx, i, false, NULL),
                          argv[idx + i]);
            if (value_is_error(err)) {
              result = err;
              goto onfinish;
            }
          }
          void *base_data = value_get_data(arr);
          value = create_comptime_slice(ctx, info->type, base_data, 0, len);
        } else {
          value_t arr =
              context_create_value(ctx, array_type, NULL, true, false, NULL);
          for (size_t i = idx; idx + i < argc; i++) {
            value_t err =
                value_set(arr, ctx, create_comptime_u64(ctx, i, false, NULL),
                          argv[idx + i]);
            if (value_is_error(err)) {
              result = err;
              goto onfinish;
            }
          }
          value =
              context_create_value(ctx, info->type, NULL, false, false, NULL);
        }
      } else {
        value = argv[idx];
        if (context_is_comptime(ctx) && !value_is_comptime(argv[idx])) {
          result =
              create_error(ctx, "argument %" PRIuPTR " is not comptime", idx);
          goto onfinish;
        }
        type_t vt = value_get_type(argv[idx]);
        if (type_get_kind(vt) == TYPE_KIND_STR) {

          if (!info->mut && type_get_kind(info->type) == TYPE_KIND_PARRAY) {
            type_t base_type = ptr_type_get_type(info->type);
            if (type_get_kind(base_type) == TYPE_KIND_UNSIGNED &&
                type_get_size(base_type) == sizeof(uint8_t)) {
              const char *src = *(const char **)value_get_data(value);
              value = context_create_value(ctx, info->type, &src, false, true,
                                           NULL);
            }
          }
          value = value_safe_convert(value, ctx, info->type);
          if (value_is_error(value)) {
            result = value;
            goto onfinish;
          }
        }
      }
      char *name = location_get(identifier->loc, allocator);
      void *data = value_get_data(value);
      value_t err = context_create_value(ctx, info->type, data, info->mut,
                                         value_is_comptime(value), name);
      allocator_free(allocator, name);
      if (value_is_error(err)) {
        result = err;
        goto onfinish;
      }
    }
    context_push_scope(ctx);
  }
  if (is_runtime) {
    function_meta_t meta = type_get_meta(function_type);
    result = context_create_value(ctx, meta->type, NULL, false, false, NULL);
  } else {
    if (!body) {
      result = create_error(ctx, "cannot call extern function in comptime");
      goto onfinish;
    }
    context_set_comptime(ctx, true);
    result = resolve_function_body(ctx, body);
  }
  if (value_is_error(result)) {
    goto onfinish;
  }
  if (value_is_interrupt(result)) {
    result = interrupt_get_value(result);
  }
onfinish:
  result = value_clone(result, allocator);
  scope_store(current_scope, allocator, NULL, result);
  context_set_scope(ctx, current_scope);
  allocator_free(allocator, scope);
  context_set_global(ctx, current_global);
  context_set_self(ctx, current_self);
  context_set_type(ctx, current_type);
  context_set_function(ctx, current_function);
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
  type_t template_func = create_type(
      allocator, TYPE_KIND_TEMPLATE, sizeof(function_declar),
      sizeof(function_declar), "template_func", "template_func", &opt, NULL);
  context_store_type(ctx, template_func);
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
value_t create_template_function(context_t ctx, ast_node_t node) {
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
  type_t type = context_load_type(ctx, "template_func");
  function_declar declar = context_store_function_declar(ctx, node, id);
  value_t func = create_value(allocator, type, false, &declar, true);
  allocator_free(allocator, id);
  module_add_function(module, func);
  return func;
}

value_t create_native_function(context_t ctx, function_handle_t handle,
                               const char *name) {
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
  const char *func_name = name;
  size_t len = strlen(parent_name) + strlen(func_name) + 2;
  char base_fullname[len + 1];
  sprintf(base_fullname, "%s_%s", parent_name, func_name);
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
  function_declar declar = context_store_native_declar(ctx, handle, id);
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