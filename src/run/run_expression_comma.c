#include "run/run.h"
#include "engine/vm.h"
#include "engine/value.h"
#include "cubec/expression_comma.h"

value_t run_expression_comma(vm_t vm, node_t node, bool shadow) {
  cubec_expression_comma_t comma = (cubec_expression_comma_t)node;

  /* Evaluate left operand (side effects), discard result */
  value_t left = run_expression(vm, comma->left, shadow);
  if (value_is_interrupt(left)) return left;
  if (value_is_abnormal(left)) return left;

  /* Evaluate right operand — this is the result */
  value_t right = run_expression(vm, comma->right, shadow);
  if (value_is_interrupt(right)) return right;
  if (value_is_abnormal(right)) return right;

  return right;
}
