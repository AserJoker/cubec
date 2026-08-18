#include "run/run.h"
#include "engine/vm.h"
#include "engine/exception_type.h"
#include "engine/value.h"
#include "cubec/expression_call.h"
#include "core/vec.h"

value_t run_expression_call(context_t ctx, node_t node, bool shadow) {
  vm_t vm = ctx->vm;
  cubec_expression_call_t call = (cubec_expression_call_t)node;

  value_t callee = run_expression(ctx, call->callee, shadow);
  if (value_is_error(callee)) return callee;

  size_t argc = vec_get_size(call->arguments);
  value_t *argv = NULL;
  if (argc > 0) {
    argv = (value_t *)allocator_alloc(ctx->allocator, argc * sizeof(value_t));
    if (!argv)
      return create_exception_value(vm, "run: out of memory for call arguments");
    for (size_t i = 0; i < argc; i++) {
      node_t arg_node = (node_t)vec_get(call->arguments, i);
      argv[i] = run_expression(ctx, arg_node, shadow);
      if (value_is_error(argv[i])) {
        value_t err = argv[i];
        allocator_free(ctx->allocator, &argv);
        return err;
      }
    }
  }

  value_t result = value_call(vm, callee, argc, argv);
  allocator_free(ctx->allocator, &argv);
  return result;
}
