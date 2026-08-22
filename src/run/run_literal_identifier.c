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
    /* TYPE_KIND_TYPE values must never be shadowed: their data carries
     * concrete type information that is always available at compile time.
     * Shadowing would lose the data (set to NULL), making type expressions
     * like typeof(i32) impossible to resolve. */
    if (type_get_kind(type) == TYPE_KIND_TYPE)
      return name->ref;
    return vm_create_value_shadow(vm, type, NULL, true);
  }

  return name->ref;
}
