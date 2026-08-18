#include "run/run.h"
#include "engine/vm.h"
#include "cubec/expression_group.h"

value_t run_expression_group(context_t ctx, node_t node, bool shadow) {
  cubec_expression_group_t group = (cubec_expression_group_t)node;
  return run_expression(ctx, group->inner, shadow);
}
