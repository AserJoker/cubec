#include "run/run.h"
#include "engine/vm.h"
#include "engine/value.h"
#include "cubec/expression_namespace_access.h"
#include "core/string.h"

value_t run_expression_namespace_access(vm_t vm, node_t node,
                                        bool shadow) {
  cubec_expression_namespace_access_t ns =
      (cubec_expression_namespace_access_t)node;

  value_t host = run_expression(vm, ns->host, shadow);
  if (value_is_abnormal(host)) return host;

  const char *field = string_get(ns->field->value);
  return value_get_prop(vm, host, field);
}
