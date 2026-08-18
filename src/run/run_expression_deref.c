#include "run/run.h"
#include "engine/vm.h"
#include "engine/value.h"
#include "cubec/expression_deref.h"

value_t run_expression_deref(context_t ctx, node_t node, bool shadow) {
  vm_t vm = ctx->vm;
  cubec_expression_deref_t deref = (cubec_expression_deref_t)node;

  value_t host = run_expression(ctx, deref->host, shadow);
  if (value_is_error(host)) return host;

  return value_deref_get(vm, host);
}
