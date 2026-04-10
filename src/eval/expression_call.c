#include "eval/expression_call.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/numeric.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
cubec_value_t cubec_eval_expression_call(cubec_context_t ctx,
                                         cubec_ast_node_t node) {
  cubec_ast_node_t callee_node = cubec_ast_get_child(node, "callee");
  cubec_ast_node_t arguments = cubec_ast_get_child(node, "arguments");
  cubec_value_t callee = NULL;
  size_t argc = cubec_ast_get_length(arguments);
  cubec_value_t argv[argc + 1];
  size_t offset = 0;
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (callee_node->type == CUBEC_NODE_TYPE_EXPRESSION_MEMBER) {
    cubec_ast_node_t host_node = cubec_ast_get_child(callee_node, "host");
    cubec_value_t host = cubec_eval_expression(ctx, host_node);
    if (cubec_value_is_error(host)) {
      return host;
    }
    cubec_ast_node_t field_node = cubec_ast_get_child(callee_node, "field");
    char *field = cubec_location_get(field_node->loc, allocator);
    callee = cubec_value_get_field(host, ctx, field);
    cubec_allocator_free(allocator, field);
    if (cubec_value_is_error(callee)) {
      return cubec_convert_compile_error(ctx, callee_node, callee);
    }
    if (!cubec_value_type_is(host, CUBEC_VALUE_TYPE_TYPE)) {
      argc++;
      offset++;
      argv[0] = host;
    }
  } else if (callee_node->type == CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER) {
    cubec_ast_node_t host_node = cubec_ast_get_child(callee_node, "host");
    cubec_value_t host = cubec_eval_expression(ctx, host_node);
    if (cubec_value_is_error(host)) {
      return host;
    }
    cubec_ast_node_t field_node = cubec_ast_get_child(callee_node, "field");
    cubec_value_t field = cubec_eval_expression(ctx, field_node);
    if (cubec_value_is_error(field)) {
      return field;
    }
    if (!cubec_value_get_data(field)) {
      return cubec_create_compile_error(ctx, field_node,
                                        "value is not comptime");
    }
    if (cubec_value_type_is(field, CUBEC_VALUE_TYPE_STR)) {
      const char *field_name = *(const char **)cubec_value_get_data(field);
      callee = cubec_value_get_field(host, ctx, field_name);
      if (cubec_value_is_error(callee)) {
        return cubec_convert_compile_error(ctx, callee_node, callee);
      }
      if (!cubec_value_type_is(host, CUBEC_VALUE_TYPE_TYPE)) {
        argc++;
        offset++;
        argv[0] = host;
      }
    } else if (cubec_value_type_is(field, CUBEC_VALUE_TYPE_UINT64)) {
      u64_t field_idx = *(u64_t *)cubec_value_get_data(field);
      callee = cubec_value_get_index(host, ctx, field_idx);
      if (cubec_value_is_error(callee)) {
        return cubec_convert_compile_error(ctx, callee_node, callee);
      }
    }
  } else {
    callee = cubec_eval_expression(ctx, callee_node);
    if (cubec_value_is_error(callee)) {
      return callee;
    }
  }
  for (size_t idx = 0; idx < cubec_ast_get_length(arguments); idx++) {
    cubec_ast_node_t arg_node = cubec_ast_get_item(arguments, idx);
    cubec_value_t arg = cubec_eval_expression(ctx, arg_node);
    if (cubec_value_is_error(arg)) {
      return arg;
    }
    argv[idx + offset] = arg;
  }
  cubec_value_t res = cubec_value_call(callee, ctx, argc, argv);
  if (cubec_value_is_error(res)) {
    return cubec_convert_compile_error(ctx, node, res);
  }
  return res;
}