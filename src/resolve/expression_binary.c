#include "resolve/expression_binary.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/location.h"
#include "core/position.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/function.h"
#include "engine/numeric.h"
#include "engine/ptr.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/union.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include <stdbool.h>
value_t resolve_expression_try(context_t ctx, value_t value, ast_node_t node) {
  static_scope_t scope = context_get_static_scope(ctx);
  if (!value_type_is(scope->binding, VALUE_TYPE_FUNCTION)) {
    return create_compile_error(ctx, node,
                                "try expression only used in function");
  }
  type_t return_type = function_type_get_type(value_get_type(scope->binding));
  if (type_get_kind(return_type) != VALUE_TYPE_STRUCT &&
      type_get_kind(return_type) != VALUE_TYPE_UNION) {
    return create_compile_error(ctx, node,
                                "function return type is not union or struct");
  }
  value_t of_error = NULL;
  if (type_get_kind(return_type) == VALUE_TYPE_STRUCT) {
    of_error = struct_type_get_attribute(return_type, "of_error");
  } else {
    of_error = union_type_get_attribute(return_type, "of_error");
  }
  if (!of_error) {
    return create_compile_error(ctx, node,
                                "no member 'of_error' in return type");
  }
  type_t of_error_type = value_get_type(of_error);
  if (type_get_kind(of_error_type) != VALUE_TYPE_FUNCTION) {
    return create_compile_error(ctx, node, ".of_error is not function");
  }
  array_t of_error_arguments = function_type_get_arguments(of_error_type);
  if (array_get_size(of_error_arguments) != 1) {
    return create_compile_error(ctx, node, ".of_error missing error argument");
  }
  type_t dst_error_type = array_get(of_error_arguments, 0);
  type_t value_type = value_get_type(value);
  type_t self_type = NULL;
  allocator_t allocator = context_get_allocator(ctx);
  if (type_get_kind(value_type) == VALUE_TYPE_STRUCT) {
    self_type = type_get_ptr_type(value_type, ctx);
  } else if (type_get_kind(value_type) == VALUE_TYPE_UNION) {
    self_type = type_get_ptr_type(value_type, ctx);
  } else if (type_get_kind(value_type) == VALUE_TYPE_PTR) {
    self_type = value_type;
    value_type = ptr_type_get_type(self_type);
  }
  if (!self_type) {
    return create_compile_error(ctx, node, "value is not struct or union");
  }
  if (type_get_kind(value_type) == VALUE_TYPE_STRUCT) {
    {
      value_t is_error = NULL;
      if (type_get_kind(value_type) == VALUE_TYPE_STRUCT) {
        is_error = struct_type_get_attribute(value_type, "is_error");
      } else {
        is_error = union_type_get_attribute(value_type, "is_error");
      }
      if (!is_error) {
        return create_compile_error(ctx, node, "no member 'is_error' in value");
      }
      if (!value_type_is(is_error, VALUE_TYPE_FUNCTION)) {
        return create_compile_error(ctx, node,
                                    "value.is_error is not function");
      }
      type_t is_error_type = value_get_type(is_error);
      value_t vfunc_type = create_function_type(
          ctx, context_load_type(ctx, "bool"), 1, &self_type, false);
      type_t type = *(type_t *)value_get_data(vfunc_type);
      if (!type_is_safe_convert(type, is_error_type)) {
        char *dst_type_name = type_to_string(type, allocator);
        char *src_type_name = type_to_string(is_error_type, allocator);
        value_t err = create_compile_error(
            ctx, node, "cannot convert is_error '%s' to '%s'", src_type_name,
            dst_type_name);
        allocator_free(allocator, dst_type_name);
        allocator_free(allocator, src_type_name);
        return err;
      }
    }
    {
      value_t error = NULL;
      if (type_get_kind(value_type) == VALUE_TYPE_STRUCT) {
        error = struct_type_get_attribute(value_type, "error");
      } else {
        error = union_type_get_attribute(value_type, "error");
      }
      if (!error) {
        return create_compile_error(ctx, node, "no member 'error' in value");
      }
      if (!value_type_is(error, VALUE_TYPE_FUNCTION)) {
        return create_compile_error(ctx, node, "value.error is not function");
      }
      type_t error_type = value_get_type(error);
      array_t arguments = function_type_get_arguments(error_type);
      if (array_get_size(arguments) != 1) {
        return create_compile_error(ctx, node,
                                    "value.error missing self argument");
      }
      type_t dst_type = array_get(arguments, 0);
      if (!type_is_safe_convert(self_type, dst_type)) {
        char *dst_type_name = type_to_string(dst_type, allocator);
        char *src_type_name = type_to_string(self_type, allocator);
        value_t err = create_compile_error(
            ctx, node, "cannot convert error self argument '%s' to '%s'",
            src_type_name, dst_type_name);
        allocator_free(allocator, dst_type_name);
        allocator_free(allocator, src_type_name);
        return err;
      }
      type_t src_error_type = function_type_get_type(error_type);
      if (!type_is_safe_convert(src_error_type, dst_error_type)) {
        char *dst_type_name = type_to_string(src_error_type, allocator);
        char *src_type_name = type_to_string(src_error_type, allocator);
        value_t err =
            create_compile_error(ctx, node, "cannot convert '%s' to '%s'",
                                 src_type_name, dst_type_name);
        allocator_free(allocator, dst_type_name);
        allocator_free(allocator, src_type_name);
        return err;
      }
    }
    {
      value_t unwrap = NULL;
      if (type_get_kind(value_type) == VALUE_TYPE_STRUCT) {
        unwrap = struct_type_get_attribute(value_type, "unwrap");
      } else {
        unwrap = union_type_get_attribute(value_type, "unwrap");
      }
      if (!unwrap) {
        return create_compile_error(ctx, node, "no member 'unwrap' in value");
      }
      if (!value_type_is(unwrap, VALUE_TYPE_FUNCTION)) {
        return create_compile_error(ctx, node, "value.unwrap is not function");
      }
      type_t unwrap_type = value_get_type(unwrap);
      array_t arguments = function_type_get_arguments(unwrap_type);
      if (array_get_size(arguments) != 1) {
        return create_compile_error(ctx, node,
                                    "value.unwrap missing self argument");
      }
      type_t dst_type = array_get(arguments, 0);
      if (!type_is_safe_convert(self_type, dst_type)) {
        char *dst_type_name = type_to_string(dst_type, allocator);
        char *src_type_name = type_to_string(self_type, allocator);
        value_t err = create_compile_error(
            ctx, node, "cannot convert unwrap self argument '%s' to '%s'",
            src_type_name, dst_type_name);
        allocator_free(allocator, dst_type_name);
        allocator_free(allocator, src_type_name);
        return err;
      }
      type_t value_type = function_type_get_type(unwrap_type);
      return context_create_value(ctx, value_type, false, NULL, NULL);
    }
  }
  return create_compile_error(ctx, node, "value is not struct or union");
}

value_t resolve_expression_binary(context_t ctx, ast_node_t node) {
  ast_node_t left_node = ast_get_child(node, "left");
  ast_node_t right_node = ast_get_child(node, "right");
  ast_node_t opt = ast_get_child(node, "opt");
  allocator_t allocator = context_get_allocator(ctx);
  if (left_node && right_node) {
    value_t lvalue = resolve_expression(ctx, left_node);
    if (value_is_error(lvalue)) {
      return lvalue;
    }
    value_t rvalue = resolve_expression(ctx, right_node);
    if (value_is_error(rvalue)) {
      return rvalue;
    }
    type_t ltype = value_get_type(lvalue);
    type_t rtype = value_get_type(rvalue);
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind < rkind) {
      if (!type_is_safe_convert(ltype, rtype)) {
        char *dst_type_name = type_to_string(rtype, allocator);
        char *src_type_name = type_to_string(ltype, allocator);
        value_t err =
            create_compile_error(ctx, left_node, "cannot convert '%s' to '%s'",
                                 src_type_name, dst_type_name);
        allocator_free(allocator, dst_type_name);
        allocator_free(allocator, src_type_name);
        return err;
      }
      ltype = rtype;
      lkind = rkind;
    } else if (lkind > rkind) {
      if (!type_is_safe_convert(rtype, ltype)) {
        char *dst_type_name = type_to_string(ltype, allocator);
        char *src_type_name = type_to_string(rtype, allocator);
        value_t err =
            create_compile_error(ctx, right_node, "cannot convert '%s' to '%s'",
                                 src_type_name, dst_type_name);
        allocator_free(allocator, dst_type_name);
        allocator_free(allocator, src_type_name);
        return err;
      }
    }
    type_operator_t op = type_get_operator(ltype);
    if (location_is(opt->loc, "+")) {
      if (op->add_opt) {
        return context_create_value(ctx, ltype, false, NULL, NULL);
      }
    } else if (location_is(opt->loc, "-")) {
      if (op->sub_opt) {
        return context_create_value(ctx, ltype, false, NULL, NULL);
      }
    } else if (location_is(opt->loc, "*")) {
      if (op->mul_opt) {
        return context_create_value(ctx, ltype, false, NULL, NULL);
      }
    } else if (location_is(opt->loc, "/")) {
      if (op->div_opt) {
        return context_create_value(ctx, ltype, false, NULL, NULL);
      }
    } else if (location_is(opt->loc, "%")) {
      if (op->mod_opt) {
        return context_create_value(ctx, ltype, false, NULL, NULL);
      }
    } else if (location_is(opt->loc, "&")) {
      if (op->and_opt) {
        return context_create_value(ctx, ltype, false, NULL, NULL);
      }
    } else if (location_is(opt->loc, "|")) {
      if (op->or_opt) {
        return context_create_value(ctx, ltype, false, NULL, NULL);
      }
    } else if (location_is(opt->loc, "^")) {
      if (op->xor_opt) {
        return context_create_value(ctx, ltype, false, NULL, NULL);
      }
    } else if (location_is(opt->loc, "<<")) {
      if (op->shl_opt) {
        return context_create_value(ctx, ltype, false, NULL, NULL);
      }
    } else if (location_is(opt->loc, ">>")) {
      if (op->shr_opt) {
        return context_create_value(ctx, ltype, false, NULL, NULL);
      }
    } else if (location_is(opt->loc, "&&")) {
      if (op->logical_and_opt) {
        return context_create_value(ctx, ltype, false, NULL, NULL);
      }
    } else if (location_is(opt->loc, "||")) {
      if (op->logical_or_opt) {
        return context_create_value(ctx, ltype, false, NULL, NULL);
      }
    } else if (location_is(opt->loc, "==")) {
      if (op->eq_opt) {
        return context_create_value(ctx, context_load_type(ctx, "bool"), false,
                                    NULL, NULL);
      }
    } else if (location_is(opt->loc, "!=")) {
      if (op->ne_opt) {
        return context_create_value(ctx, context_load_type(ctx, "bool"), false,
                                    NULL, NULL);
      }
    } else if (location_is(opt->loc, ">")) {
      if (op->gt_opt) {
        return context_create_value(ctx, context_load_type(ctx, "bool"), false,
                                    NULL, NULL);
      }
    } else if (location_is(opt->loc, "<")) {
      if (op->lt_opt) {
        return context_create_value(ctx, context_load_type(ctx, "bool"), false,
                                    NULL, NULL);
      }
    } else if (location_is(opt->loc, ">=")) {
      if (op->ge_opt) {
        return context_create_value(ctx, context_load_type(ctx, "bool"), false,
                                    NULL, NULL);
      }
    } else if (location_is(opt->loc, "<=")) {
      if (op->le_opt) {
        return context_create_value(ctx, context_load_type(ctx, "bool"), false,
                                    NULL, NULL);
      }
    }
  } else if (right_node) {
    value_t value = resolve_expression(ctx, right_node);
    if (value_is_error(value)) {
      return value;
    }
    type_t type = value_get_type(value);
    type_kind_t kind = type_get_kind(type);
    type_operator_t op = type_get_operator(type);
    if (location_is(opt->loc, "+")) {
      if (op->plus_opt) {
        return context_create_value(ctx, type, false, NULL, NULL);
      }
    } else if (location_is(opt->loc, "-")) {
      if (op->neg_opt) {
        return context_create_value(ctx, type, false, NULL, NULL);
      }
    } else if (location_is(opt->loc, "!")) {
      if (op->logical_not_opt) {
        return context_create_value(ctx, type, false, NULL, NULL);
      }
    } else if (location_is(opt->loc, "&")) {
      if (op->ref) {
        return context_create_value(ctx, type_get_ptr_type(type, ctx), false,
                                    NULL, NULL);
      }
    } else if (location_is(opt->loc, "*")) {
      if (op->unref) {
        return context_create_value(ctx, ptr_type_get_type(type), false, NULL,
                                    NULL);
      }
    } else if (location_is(opt->loc, "sizeof")) {
      return create_i64(ctx, type_get_size(type), false, NULL);
    } else if (location_is(opt->loc, "alignof")) {
      return create_i64(ctx, type_get_align(type), false, NULL);
    } else if (location_is(opt->loc, "typeof")) {
      return create_type_value(ctx, type, false, NULL);
    } else if (location_is(opt->loc, "try")) {
      return resolve_expression_try(ctx, value, right_node);
    }
  }
  return create_compile_error(ctx, node, "unsupprort binary expression");
}