#include "run/run.h"
#include "engine/vm.h"
#include "engine/value.h"
#include "cubec/expression_addr.h"

value_t run_expression_addr(vm_t vm, node_t node, bool shadow) {
  cubec_expression_addr_t addr = (cubec_expression_addr_t)node;

  value_t host = run_expression(vm, addr->host, shadow);
  if (value_is_abnormal(host)) return host;

  return value_addrof(vm, host);
}
