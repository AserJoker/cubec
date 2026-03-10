#include "runtime/program.h"
#include "ast/node.h"
#include "core/list.h"
#include "engine/context.h"
#include "engine/type.h"
#include "runtime/vm.h"
cubec_value_t cubec_run_program_node(cubec_context_t ctx, cubec_vm_t vm,
                                     cubec_ast_program_t node) {
  cubec_list_node_t it = cubec_list_get_first(node->statements);
  while (it != cubec_list_get_end(node->statements)) {
    cubec_ast_node_t node = cubec_list_node_get(it);
    cubec_value_t expr = cubec_vm_run(vm, ctx, node);
    if (expr->type->kind == CUBEC_VALUE_TYPE_ERROR) {
      return expr;
    }
    it = cubec_list_node_next(it);
  }
  return cubec_context_create_undefined(ctx, NULL);
}