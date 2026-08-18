#include "run/run.h"
#include "engine/vm.h"
#include "engine/value.h"
#include "cubec/expression_addr.h"

value_t run_expression_addr(context_t ctx, node_t node, bool shadow) {
  vm_t vm = ctx->vm;
  cubec_expression_addr_t addr = (cubec_expression_addr_t)node;

  value_t host = run_expression(ctx, addr->host, shadow);
  if (value_is_error(host)) return host;

  return value_addrof(vm, host);
}
