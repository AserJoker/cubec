#include "runtime/expression_binary.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include "runtime/vm.h"
cubec_value_t cubec_run_expression_binary(cubec_context_t ctx, cubec_vm_t vm,
                                          cubec_ast_expression_binary_t node) {
  cubec_ast_node_t opt = node->opt;
  if (node->left && node->right) {
    cubec_value_t left = cubec_vm_run(vm, ctx, node->left);
    cubec_value_t right = cubec_vm_run(vm, ctx, node->right);
  } else if (node->left) {

  } else if (node->right) {
    cubec_value_t right = cubec_vm_run(vm, ctx, node->right);
    if (right->type->kind == CUBEC_VALUE_TYPE_ERROR) {
      return right;
    }
    if (cubec_location_is(opt->loc, "*")) {
      if (right->type == ctx->named_types.type_type) {
        cubec_type_t type = right->data;
        type = cubec_context_get_ptr_type(ctx, type);
        return cubec_context_create_type_value(ctx, type, NULL);
      }
      if (right->type->kind == CUBEC_VALUE_TYPE_PTR) {
        right = *(cubec_value_t *)right->data;
        return cubec_context_create_ref(ctx, right, NULL);
      }
    }
    if (cubec_location_is(opt->loc, "&")) {
      if (right->type == ctx->named_types.type_type) {
        cubec_type_t type = right->data;
        type = cubec_context_get_ref_type(ctx, type);
        return cubec_context_create_type_value(ctx, type, NULL);
      } else {
        return cubec_context_create_ptr(ctx, right, NULL);
      }
    }
    if (cubec_location_is(opt->loc, "!")) {
      right = cubec_context_to_boolean(ctx, right);
      if (right->type->kind == CUBEC_VALUE_TYPE_ERROR) {
        return right;
      }
      return cubec_context_create_boolean(ctx, !*(bool *)right->data, NULL);
    }
  }
  return cubec_context_create_error(ctx, "Not implement", NULL);
}