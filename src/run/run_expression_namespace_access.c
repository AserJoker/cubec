#include "run/run.h"
#include "engine/vm.h"
#include "engine/value.h"
#include "cubec/expression_namespace_access.h"
#include "core/string.h"

value_t run_expression_namespace_access(context_t ctx, node_t node,
                                        bool shadow) {
  vm_t vm = ctx->vm;
  cubec_expression_namespace_access_t ns =
      (cubec_expression_namespace_access_t)node;

  value_t host = run_expression(ctx, ns->host, shadow);
  if (value_is_error(host)) return host;

  const char *field = string_get(ns->field->value);
  return value_get_prop(vm, host, field);
}
