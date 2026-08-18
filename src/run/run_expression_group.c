#include "run/run.h"
#include "engine/vm.h"
#include "cubec/expression_group.h"

value_t run_expression_group(vm_t vm, node_t node, bool shadow) {
  cubec_expression_group_t group = (cubec_expression_group_t)node;
  return run_expression(vm, group->inner, shadow);
}
