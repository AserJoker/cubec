#include "resolve/expression_call.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/function.h"
#include "engine/ptr.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/union.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include <inttypes.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <string.h>

static value_t resolve_function_call(context_t ctx, value_t callee,
                                     value_t self, ast_node_t self_node,
                                     size_t argc, value_t *argv,
                                     ast_node_t arguments) {
  type_t fn_type = value_get_type(callee);
  allocator_t allocator = context_get_allocator(ctx);
  bool is_variadic = function_type_is_variadic(fn_type);
  array_t func_arguments = function_type_get_arguments(fn_type);
  type_t result_type = function_type_get_type(fn_type);
  type_t variadic_type = NULL;
  if (is_variadic) {
    variadic_type =
        array_get(func_arguments, array_get_size(func_arguments) - 1);
  }
  size_t offset = 0;
  if (self) {
    offset = 1;
  }
  for (size_t idx = 0; idx < argc; idx++) {
    type_t arg_type = value_get_type(argv[idx]);
    if (is_variadic) {
      if (idx + offset >= array_get_size(func_arguments) - 1) {
        if (!is_variadic) {
          return create_compile_error(
              ctx, ast_get_item(arguments, idx),
              "function required %" PRIuPTR " arguments receive %" PRIuPTR,
              array_get_size(func_arguments), argc + offset);
        }
        if (!type_is_safe_convert(variadic_type, arg_type)) {
          char *dst_type_name = type_to_string(variadic_type, allocator);
          char *src_type_name = type_to_string(arg_type, allocator);
          value_t err = create_compile_error(
              ctx, ast_get_item(arguments, idx),
              "cannot convert '%s' to '%s' for argument %" PRIuPTR,
              src_type_name, dst_type_name, idx + offset);
          allocator_free(allocator, dst_type_name);
          allocator_free(allocator, src_type_name);
          return err;
        }
      } else {
        type_t dst_arg_type = array_get(func_arguments, idx + offset);
        if (!type_is_safe_convert(dst_arg_type, arg_type)) {
          char *dst_type_name = type_to_string(dst_arg_type, allocator);
          char *src_type_name = type_to_string(arg_type, allocator);
          value_t err = create_compile_error(
              ctx, ast_get_item(arguments, idx),
              "cannot convert '%s' to '%s' for argument %" PRIuPTR,
              src_type_name, dst_type_name, idx + offset);
          allocator_free(allocator, dst_type_name);
          allocator_free(allocator, src_type_name);
          return err;
        }
      }
    } else {
      type_t dst_arg_type = array_get(func_arguments, idx + offset);
      if (!type_is_safe_convert(dst_arg_type, arg_type)) {
        char *dst_type_name = type_to_string(dst_arg_type, allocator);
        char *src_type_name = type_to_string(arg_type, allocator);
        value_t err = create_compile_error(
            ctx, ast_get_item(arguments, idx),
            "cannot convert '%s' to '%s' for argument %" PRIuPTR, src_type_name,
            dst_type_name, idx + offset);
        allocator_free(allocator, dst_type_name);
        allocator_free(allocator, src_type_name);
        return err;
      }
    }
  }
  if (self) {
    type_t self_type = value_get_type(self);
    if (!array_get_size(func_arguments)) {
      return create_compile_error(ctx, self_node,
                                  "function required %" PRIuPTR
                                  " arguments receive %" PRIuPTR,
                                  array_get_size(func_arguments), 1);
    } else {
      type_t arg_type = array_get(func_arguments, 0);
      if (!type_is_safe_convert(arg_type, self_type)) {
        char *dst_type_name = type_to_string(arg_type, allocator);
        char *src_type_name = type_to_string(self_type, allocator);
        value_t err = create_compile_error(
            ctx, self_node,
            "cannot convert '%s' to '%s' for argument %" PRIuPTR, src_type_name,
            dst_type_name, 0);
        allocator_free(allocator, dst_type_name);
        allocator_free(allocator, src_type_name);
        return err;
      }
    }
  }
  return context_create_value(ctx, result_type, false, NULL, NULL);
}

value_t resolve_expression_call(context_t ctx, ast_node_t node) {
  ast_node_t callee_node = ast_get_child(node, "callee");
  ast_node_t arguments = ast_get_child(node, "arguments");
  size_t argc = ast_get_length(arguments);
  value_t argv[argc];
  for (size_t idx = 0; idx < argc; idx++) {
    ast_node_t item = ast_get_item(arguments, idx);
    value_t arg = resolve_expression(ctx, item);
    if (value_is_error(arg)) {
      return arg;
    }
    argv[idx] = arg;
  }
  if (callee_node->type == NODE_TYPE_EXPRESSION_MEMBER) {
    ast_node_t host_node = ast_get_child(callee_node, "host");
    ast_node_t field_node = ast_get_child(callee_node, "field");
    value_t host = resolve_expression(ctx, host_node);
    if (value_is_error(host)) {
      return host;
    }
    allocator_t allocator = context_get_allocator(ctx);
    char *field = location_get(field_node->loc, allocator);
    if (value_is_comptime(host)) {
      for (size_t idx = 0; idx < argc; idx++) {
        if (!value_is_comptime(argv[idx])) {
          return create_compile_error(ctx, ast_get_item(arguments, idx),
                                      "expression is not comptime");
        }
      }
      value_t res = value_member_call(host, ctx, field, argc, argv);
      allocator_free(allocator, field);
      if (value_is_error(res)) {
        return convert_compile_error(ctx, node, res);
      }
      return res;
    } else {
      type_t value_type = value_get_type(host);
      type_t self_type = NULL;
      if (type_get_kind(value_type) == VALUE_TYPE_PTR) {
        self_type = value_type;
        value_type = ptr_type_get_type(self_type);
      } else {
        self_type = type_get_ptr_type(value_type, ctx);
      }
      if (type_get_kind(value_type) == VALUE_TYPE_STRUCT) {
        value_t callee = struct_type_get_attribute(value_type, field);
        if (!callee) {
          value_t error =
              create_compile_error(ctx, host_node, "no member %s in value");
          allocator_free(allocator, field);
          return error;
        }
        if (!value_type_is(callee, VALUE_TYPE_FUNCTION)) {
          value_t error = create_compile_error(ctx, host_node,
                                               "value.%s is not a function");
          allocator_free(allocator, field);
          return error;
        }
        allocator_free(allocator, field);
        if (value_is_comptime(callee)) {
          return create_compile_error(ctx, host_node,
                                      "expression is not comptime");
        }
        value_t self = context_create_value(ctx, self_type, false, NULL, NULL);
        return resolve_function_call(ctx, callee, self, host_node, argc, argv,
                                     arguments);
      } else if (type_get_kind(value_type) == VALUE_TYPE_UNION) {
        value_t callee = union_type_get_attribute(value_type, field);
        if (!callee) {
          value_t error =
              create_compile_error(ctx, host_node, "no member %s in value");
          allocator_free(allocator, field);
          return error;
        }
        if (!value_type_is(callee, VALUE_TYPE_FUNCTION)) {
          value_t error = create_compile_error(ctx, host_node,
                                               "value.%s is not a function");
          allocator_free(allocator, field);
          return error;
        }
        allocator_free(allocator, field);
        if (value_is_comptime(callee)) {
          return create_compile_error(ctx, host_node,
                                      "expression is not comptime");
        }
        value_t self = context_create_value(ctx, self_type, false, NULL, NULL);
        return resolve_function_call(ctx, callee, self, host_node, argc, argv,
                                     arguments);
      } else {
        allocator_free(allocator, field);
        return create_compile_error(ctx, host_node,
                                    "expression is not struct or union");
      }
    }
    allocator_free(allocator, field);
  } else {
    value_t callee = resolve_expression(ctx, callee_node);
    if (value_type_is(callee, VALUE_TYPE_FUNCTION)) {
      if (value_is_comptime(callee)) {
        for (size_t idx = 0; idx < argc; idx++) {
          if (!value_is_comptime(argv[idx])) {
            return create_compile_error(ctx, ast_get_item(arguments, idx),
                                        "expression is not comptime");
          }
        }
        return value_call(callee, ctx, argc, argv);
      } else {
        return resolve_function_call(ctx, callee, NULL, NULL, argc, argv,
                                     arguments);
      }
    } else if (value_type_is(callee, VALUE_TYPE_BUILTIN)) {
      return value_call(callee, ctx, argc, argv);
    } else {
      return create_compile_error(ctx, callee_node,
                                  "expression is not callable");
    }
  }
}