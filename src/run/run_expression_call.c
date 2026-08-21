#include "run/run.h"
#include "engine/vm.h"
#include "engine/exception_type.h"
#include "engine/value.h"
#include "cubec/expression_call.h"
#include "cubec/expression_spread.h"
#include "core/vec.h"

value_t run_expression_call(vm_t vm, node_t node, bool shadow) {
  cubec_expression_call_t call = (cubec_expression_call_t)node;

  value_t callee = run_expression(vm, call->callee, shadow);
  if (value_is_abnormal(callee)) return callee;

  /* Evaluate arguments, expanding spread nodes into individual values.
   * First pass: count total expanded argc. Second pass: fill argv. */
  size_t ast_argc = vec_get_size(call->arguments);
  allocator_t alloc = vm_get_allocator(vm);

  /* Collect evaluated arguments into a vec first (to handle variable-length
   * expansion from spread). Each non-spread arg adds 1 value;
   * each spread arg adds N values from value_spread(). */
  vec_init_t avi = {.auto_dispose = false};
  vec_t arg_vec = (vec_t)allocator_create(alloc, &g_vec_class, &avi);

  for (size_t i = 0; i < ast_argc; i++) {
    node_t arg_node = (node_t)vec_get(call->arguments, i);

    if (arg_node->kind == CUBEC_NODE_EXPRESSION_SPREAD) {
      /* spread: evaluate inner, call value_spread, push each element */
      cubec_expression_spread_t spread = (cubec_expression_spread_t)arg_node;
      value_t inner = run_expression(vm, spread->value, shadow);
      if (value_is_abnormal(inner)) {
        allocator_free(alloc, &arg_vec);
        return inner;
      }
      vec_t expanded = value_spread(vm, inner);
      if (!expanded) {
        allocator_free(alloc, &arg_vec);
        return create_exception_value(vm,
            "cannot spread value of type '%s' in function call",
            type_get_name(value_get_type(inner)));
      }
      size_t exp_count = vec_get_size(expanded);
      for (size_t j = 0; j < exp_count; j++)
        vec_push(arg_vec, vec_get(expanded, j));
      allocator_free(alloc, &expanded);
    } else {
      /* normal argument */
      value_t v = run_expression(vm, arg_node, shadow);
      if (value_is_abnormal(v)) {
        allocator_free(alloc, &arg_vec);
        return v;
      }
      vec_push(arg_vec, v);
    }
  }

  size_t argc = vec_get_size(arg_vec);
  value_t *argv = NULL;
  if (argc > 0) {
    argv = (value_t *)allocator_alloc(alloc, argc * sizeof(value_t));
    for (size_t i = 0; i < argc; i++)
      argv[i] = (value_t)vec_get(arg_vec, i);
  }
  allocator_free(alloc, &arg_vec);

  value_t result = value_call(vm, callee, argc, argv);
  allocator_free(alloc, &argv);
  return result;
}
