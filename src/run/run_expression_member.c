#include "run/run.h"
#include "engine/vm.h"
#include "engine/value.h"
#include "cubec/expression_member.h"
#include "cubec/literal_identifier.h"
#include "core/string.h"

value_t run_expression_member(vm_t vm, node_t node, bool shadow) {
  cubec_expression_member_t mem = (cubec_expression_member_t)node;

  value_t host = run_expression(vm, mem->host, shadow);
  if (value_is_abnormal(host)) return host;

  const char *field = string_get(mem->field->value);
  return value_get_field(vm, host, field);
}
