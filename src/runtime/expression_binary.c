#include "runtime/expression_binary.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/value.h"
#include "runtime/vm.h"

cubec_value_t cubec_run_expression_binary(cubec_context_t ctx, cubec_vm_t vm,
                                          cubec_ast_expression_binary_t node) {
  cubec_ast_node_t opt = node->opt;
  if (cubec_location_is(opt->loc, "*")) {
    if (!node->left) {
      cubec_value_t right = cubec_vm_run(vm, ctx, node->right);
      if (right->type == ctx->named_types.type_type) {
        cubec_type_t type = right->data;
        type = cubec_context_get_ptr_type(ctx, type);
        return cubec_context_create_type_value(ctx, type, NULL);
      }
    }
  }
  if (cubec_location_is(opt->loc, "&")) {
    if (!node->left) {
      cubec_value_t right = cubec_vm_run(vm, ctx, node->right);
      if (right->type == ctx->named_types.type_type) {
        cubec_type_t type = right->data;
        type = cubec_context_get_ref_type(ctx, type);
        return cubec_context_create_type_value(ctx, type, NULL);
      }
    }
  }
  return cubec_context_create_error(ctx, "Not implement", NULL);
}