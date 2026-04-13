#include "eval/expression_assigment.h"
#include "ast/expression_group.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
#include "eval/literal_identifier.h"
#include <stdint.h>
#include <string.h>
cubec_value_t cubec_eval_expression_assigment(cubec_context_t ctx,
                                              cubec_ast_node_t node) {
  cubec_ast_node_t identifier_node = cubec_ast_get_child(node, "identifier");
  identifier_node = cubec_ast_unwrap_group(identifier_node);
  cubec_ast_node_t value_node = cubec_ast_get_child(node, "value");
  cubec_ast_node_t opt = cubec_ast_get_child(node, "opt");
  cubec_value_t value = cubec_eval_expression(ctx, value_node);
  if (cubec_value_is_error(value)) {
    return value;
  }
  cubec_value_t dst = NULL;
  cubec_value_t ptr = NULL;
  cubec_value_t host = NULL;
  char *field = NULL;
  size_t idx = 0;
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (identifier_node->type == CUBEC_NODE_TYPE_EXPRESSION_MEMBER) {
    cubec_ast_node_t host_node = cubec_ast_get_child(identifier_node, "host");
    cubec_ast_node_t field_node = cubec_ast_get_child(identifier_node, "field");
    host = cubec_eval_expression(ctx, host_node);
    if (cubec_value_is_error(host)) {
      return host;
    }
    field = cubec_location_get(field_node->loc, allocator);
    dst = cubec_value_get_field(host, ctx, field);
  } else if (identifier_node->type ==
             CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER) {
    cubec_ast_node_t host_node = cubec_ast_get_child(identifier_node, "host");
    cubec_ast_node_t field_node = cubec_ast_get_child(identifier_node, "field");
    host = cubec_eval_expression(ctx, host_node);
    if (cubec_value_is_error(host)) {
      return host;
    }
    cubec_value_t field_value = cubec_eval_expression(ctx, field_node);
    if (cubec_value_is_error(field_value)) {
      return field_value;
    }
    if (!cubec_value_get_data(field_value)) {
      return cubec_create_compile_error(ctx, field_node,
                                        "expression is not comptime");
    }
    if (cubec_value_type_is(field_value, CUBEC_VALUE_TYPE_STR)) {
      field = cubec_create_cstring(
          allocator, *(const char **)cubec_value_get_data(field_value));
      dst = cubec_value_get_field(host, ctx, field);
    } else {
      cubec_type_t type = cubec_value_get_type(field_value);
      cubec_type_kind_t kind = cubec_type_get_kind(type);
      if (kind < CUBEC_VALUE_TYPE_INT8 || kind > CUBEC_VALUE_TYPE_UINT64) {
        return cubec_create_compile_error(ctx, field_node, "invalid index");
      }
      if (kind >= CUBEC_VALUE_TYPE_INT8 && kind <= CUBEC_VALUE_TYPE_INT64) {
        field_value = cubec_value_safe_convert(
            value, ctx, cubec_context_load_type(ctx, "i64"));
        if (cubec_value_is_error(field_value)) {
          return cubec_convert_compile_error(ctx, field_node, field_value);
        }
        int64_t val = *(int64_t *)cubec_value_get_data(field_value);
        if (val < 0) {
          return cubec_create_compile_error(ctx, field_node, "invalid index");
        }
        idx = val;
      } else {
        field_value = cubec_value_safe_convert(
            value, ctx, cubec_context_load_type(ctx, "u64"));
        if (cubec_value_is_error(field_value)) {
          return cubec_convert_compile_error(ctx, field_node, field_value);
        }
        idx = *(uint64_t *)cubec_value_get_data(field_value);
      }
      dst = cubec_value_get_index(host, ctx, idx);
    }
  } else if (identifier_node->type == CUBEC_NODE_TYPE_EXPRESSION_BINARY) {
    cubec_ast_node_t opt = cubec_ast_get_child(identifier_node, "opt");
    cubec_ast_node_t left = cubec_ast_get_child(identifier_node, "left");
    cubec_ast_node_t right = cubec_ast_get_child(identifier_node, "right");
    if (!left && right && cubec_location_is(opt->loc, "*")) {
      ptr = cubec_eval_expression(ctx, right);
      if (cubec_value_is_error(ptr)) {
        return ptr;
      }
      dst = cubec_value_unref(ptr, ctx);
    }
  } else if (identifier_node->type == CUBEC_NODE_TYPE_LITERAL_IDENTIFIER) {
    dst = cubec_eval_literal_identifier(ctx, identifier_node);
  }
  if (!dst) {
    return cubec_create_compile_error(ctx, identifier_node,
                                      "expression is not assigmentable");
  }
  if (cubec_value_is_error(dst)) {
    return dst;
  }
  if (cubec_location_is(opt->loc, "+=")) {
    value = cubec_value_add(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, "-=")) {
    value = cubec_value_sub(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, "*=")) {
    value = cubec_value_mul(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, "/=")) {
    value = cubec_value_div(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, "%=")) {
    value = cubec_value_mod(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, "&=")) {
    value = cubec_value_and(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, "|=")) {
    value = cubec_value_or(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, "^=")) {
    value = cubec_value_xor(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, ">>=")) {
    value = cubec_value_shr(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, "<<=")) {
    value = cubec_value_shl(dst, ctx, value);
  } else if (!cubec_location_is(opt->loc, "=")) {
    return cubec_create_compile_error(ctx, node,
                                      "unsupport assigment operator");
  }
  if (cubec_value_is_error(value)) {
    return cubec_convert_compile_error(ctx, value_node, value);
  }
  cubec_value_t res = value;
  if (ptr) {
    res = cubec_value_unref_assigment(ptr, ctx, value);
  } else if (host) {
    if (field) {
      res = cubec_value_set_field(host, ctx, field, value);
      cubec_allocator_free(allocator, field);
    } else {
      res = cubec_value_set_index(host, ctx, idx, value);
    }
  } else {
    res = cubec_value_assigment(dst, ctx, value);
  }
  if (cubec_value_is_error(res)) {
    return cubec_convert_compile_error(ctx, node, res);
  }
  return res;
}