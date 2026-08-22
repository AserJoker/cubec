#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/diagnostic.h"
#include "engine/value.h"
#include "engine/type.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/cunion_type.h"
#include "cubec/statement_cunion.h"
#include "cubec/struct_field.h"
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

value_t run_statement_cunion(vm_t vm, node_t node, bool shadow) {
  (void)shadow; /* cunion type expressions evaluate identically in both modes */
  cubec_statement_cunion_t stmt = (cubec_statement_cunion_t)node;
  const char *name = _get_name(stmt->name);

  /* create the cunion type value (unsealed) */
  value_t type_val = vm_create_cunion_type_value(vm, name, false,
                          vm_get_current_module_id(vm));

  /* add fields */
  vec_t fields = stmt->fields;
  size_t fc = vec_get_size(fields);
  for (size_t i = 0; i < fc; i++) {
    cubec_struct_field_t field = (cubec_struct_field_t)vec_get(fields, i);
    const char *field_name = _get_name(field->name);

    /* evaluate field type expression */
    value_t field_type_val = run_expression(vm, field->type, false);
    if (value_is_abnormal(field_type_val))
      return field_type_val;

    if (type_get_kind(value_get_type(field_type_val)) != TYPE_KIND_TYPE)
      return create_exception_value(vm,
          "cunion field '%s' type expression must produce a type, got '%s'",
          field_name, type_get_name(value_get_type(field_type_val)));

    value_t result = vm_cunion_add_field(vm, type_val, field_name,
                                          field_type_val, field->is_pub);
    if (value_is_abnormal(result))
      return result;
  }

  /* seal the cunion */
  value_t seal_result = vm_cunion_seal(vm, type_val);
  if (value_is_abnormal(seal_result))
    return seal_result;

  /* bind the name in current scope */
  _bind_name(vm, type_val, name);

  return create_void_value(vm);
}
