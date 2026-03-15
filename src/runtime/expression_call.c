#include "runtime/expression_call.h"
#include "ast/expression_spread.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/list.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include "runtime/vm.h"

cubec_value_t cubec_run_expression_call(cubec_context_t ctx, cubec_vm_t vm,
                                        cubec_ast_expression_call_t node) {
  cubec_value_t callee = cubec_vm_run(vm, ctx, node->callee);
  if (callee->type->kind == CUBEC_VALUE_TYPE_ERROR) {
    return callee;
  }
  cubec_array_t argv = cubec_create_array(ctx->allocator, NULL);
  for (cubec_list_node_t it = cubec_list_get_first(node->args);
       it != cubec_list_get_end(node->args); it = cubec_list_node_next(it)) {
    cubec_ast_node_t node = cubec_list_node_get(it);
    if (node->type == CUBEC_NODE_TYPE_EXPRESSION_SPREAD) {
      cubec_ast_expression_spread_t arg = (cubec_ast_expression_spread_t)node;
      cubec_value_t val = cubec_vm_run(vm, ctx, arg->expression);
      if (val->type->kind == CUBEC_VALUE_TYPE_ERROR) {
        cubec_allocator_free(ctx->allocator, argv);
        return val;
      }
      cubec_value_t length = cubec_context_get_length(ctx, val);
      if (length->type->kind == CUBEC_VALUE_TYPE_ERROR) {
        cubec_allocator_free(ctx->allocator, argv);
        return length;
      }
      length = cubec_context_convert(ctx, ctx->named_types.uint64_type, length);
      if (length->type->kind == CUBEC_VALUE_TYPE_ERROR) {
        return length;
      }
      size_t len = *(uint64_t *)length->data;
      for (size_t idx = 0; idx < len; idx++) {
        cubec_value_t item = cubec_context_get_index(ctx, val, idx);
        if (item->type->kind == CUBEC_VALUE_TYPE_ERROR) {
          cubec_allocator_free(ctx->allocator, argv);
          return item;
        }
        cubec_array_push(argv, ctx->allocator, item);
      }
    } else {
      cubec_value_t val = cubec_vm_run(vm, ctx, node);
      if (val->type->kind == CUBEC_VALUE_TYPE_ERROR) {
        cubec_allocator_free(ctx->allocator, argv);
        return val;
      }
      cubec_array_push(argv, ctx->allocator, val);
    }
  }
  cubec_value_t *args = cubec_array_get_data(argv);
  cubec_value_t res =
      cubec_context_call(ctx, callee, cubec_array_get_size(argv), args);
  cubec_allocator_free(ctx->allocator, argv);
  return res;
}