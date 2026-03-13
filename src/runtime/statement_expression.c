#include "runtime/statement_expression.h"
#include "engine/context.h"
#include "engine/type.h"
#include "runtime/vm.h"

cubec_value_t
cubec_run_statement_expression(cubec_context_t ctx, cubec_vm_t vm,
                               cubec_ast_statement_expression_t node) {
  cubec_value_t val = cubec_vm_run(vm, ctx, node->expression);
  if (val->type->kind == CUBEC_VALUE_TYPE_ERROR) {
    return val;
  }
  return cubec_context_get_undefined(ctx);
}