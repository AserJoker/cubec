#include "run/run.h"
#include "cubec/literal_identifier.h"
#include "engine/vm.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/exception_type.h"
#include "core/string.h"

value_t run_literal_identifier(vm_t vm, node_t node, bool shadow) {
  cubec_literal_identifier_t lit = (cubec_literal_identifier_t)node;
  const char *name_str = string_get(lit->value);

  scope_t scope = vm_get_current_scope(vm);
  name_t name = scope_lookup(scope, name_str);
  if (!name || !name->ref)
    return create_exception_value(vm, "undefined identifier: %s", name_str);

  if (shadow) {
    type_t type = value_get_type(name->ref);
    type_kind_t kind = type_get_kind(type);
    /* TYPE_KIND_TYPE, TYPE_KIND_GENERIC, and TYPE_KIND_GENERIC_FN values
     * must never be shadowed: their data carries concrete type information
     * (or the create_instance callback) that is always available at compile
     * time. Shadowing would lose the data (set to NULL), making type
     * expressions like Wrapper[T] impossible to resolve. */
    if (kind == TYPE_KIND_TYPE || kind == TYPE_KIND_GENERIC ||
        kind == TYPE_KIND_GENERIC_FN)
      return name->ref;
    return vm_create_value_shadow(vm, type, NULL, true);
  }

  return name->ref;
}
