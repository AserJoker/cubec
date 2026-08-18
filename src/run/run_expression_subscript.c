#include "run/run.h"
#include "engine/vm.h"
#include "engine/exception_type.h"
#include "engine/value.h"
#include "cubec/expression_subscript.h"
#include "core/vec.h"

value_t run_expression_subscript(context_t ctx, node_t node, bool shadow) {
  vm_t vm = ctx->vm;
  cubec_expression_subscript_t sub = (cubec_expression_subscript_t)node;

  value_t host = run_expression(ctx, sub->host, shadow);
  if (value_is_error(host)) return host;

  /* TODO: validate host is not a generic name (not yet implemented) */
  size_t argc = vec_get_size(sub->arguments);
  if (argc != 1)
    return create_exception_value(vm,
                                  "run: subscript requires exactly one argument, got %zu",
                                  argc);

  node_t index_node = (node_t)vec_get(sub->arguments, 0);
  value_t index = run_expression(ctx, index_node, shadow);
  if (value_is_error(index)) return index;

  return value_get_item(vm, host, index);
}
