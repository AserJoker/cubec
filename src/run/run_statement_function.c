#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "cubec/statement_function.h"
#include "cubec/declaration_function.h"
#include "cubec/literal_identifier.h"
#include "core/string.h"

/* ---- helper: extract name string from identifier node ---- */

static const char *_get_name(node_t identifier) {
  cubec_literal_identifier_t id = (cubec_literal_identifier_t)identifier;
  return string_get(id->value);
}

/* ---- helper: bind a name to an already-registered value ---- */

static void _bind_name(vm_t vm, value_t val, const char *name) {
  scope_t scope = vm_get_current_scope(vm);
  if (scope && name) {
    name_t n = name_create(scope->allocator, val);
    char *owned = cstring_clone(scope->allocator, name);
    strmap_insert(scope->names, owned, n);
    allocator_free(scope->allocator, &owned);
  }
}

/* ---- main entry ---- */

value_t run_statement_function(vm_t vm, node_t node, bool shadow) {
  cubec_statement_function_t stmt = (cubec_statement_function_t)node;
  cubec_declaration_function_t decl =
      (cubec_declaration_function_t)stmt->declarator;
  const char *name = decl->name ? _get_name(decl->name) : NULL;

  /* construct the function value via declaration runner */
  value_t func_val = run_declaration_function(vm, (node_t)decl, shadow);
  if (value_is_abnormal(func_val))
    return func_val;

  /* bind the name in current scope */
  _bind_name(vm, func_val, name);

  return create_void_value(vm);
}
