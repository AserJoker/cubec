#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/diagnostic.h"
#include "engine/value.h"
#include "cubec/node.h"

/* ---- expression dispatcher ---- */

value_t run_expression(vm_t vm, node_t node, bool shadow) {
  if (!node) return create_void_value(vm);

  value_t result;
  switch (node->kind) {
  /* literals */
  case CUBEC_NODE_LITERAL_NUMERIC:
    result = run_literal_numeric(vm, node, shadow); break;
  case CUBEC_NODE_LITERAL_STRING:
    result = run_literal_string(vm, node, shadow); break;
  case CUBEC_NODE_LITERAL_CHAR:
    result = run_literal_char(vm, node, shadow); break;
  case CUBEC_NODE_LITERAL_IDENTIFIER:
    result = run_literal_identifier(vm, node, shadow); break;
  case CUBEC_NODE_LITERAL_NIL:
    result = run_literal_nil(vm, node, shadow); break;
  case CUBEC_NODE_LITERAL_BOOL:
    result = run_literal_bool(vm, node, shadow); break;
  case CUBEC_NODE_LITERAL_UNDEFINED:
    result = run_literal_undefined(vm, node, shadow); break;
  /* expressions */
  case CUBEC_NODE_EXPRESSION_BINARY:
    result = run_expression_binary(vm, node, shadow); break;
  case CUBEC_NODE_EXPRESSION_ASSIGNMENT:
    result = run_expression_assignment(vm, node, shadow); break;
  case CUBEC_NODE_EXPRESSION_DEREF:
    result = run_expression_deref(vm, node, shadow); break;
  case CUBEC_NODE_EXPRESSION_ADDR:
    result = run_expression_addr(vm, node, shadow); break;
  case CUBEC_NODE_EXPRESSION_MEMBER:
    result = run_expression_member(vm, node, shadow); break;
  case CUBEC_NODE_EXPRESSION_CALL:
    result = run_expression_call(vm, node, shadow); break;
  case CUBEC_NODE_EXPRESSION_GROUP:
    result = run_expression_group(vm, node, shadow); break;
  case CUBEC_NODE_EXPRESSION_SUBSCRIPT:
    /* subscript covers the unified [...] syntax; semantic analysis
     * decides whether host[args] is a subscript or generic instantiation,
     * but at run time runtime values use subscript (value_get_item). */
    result = run_expression_subscript(vm, node, shadow); break;
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS:
    result = run_expression_namespace_access(vm, node, shadow); break;
  case CUBEC_NODE_EXPRESSION_INITIALIZE_LIST:
    result = run_expression_initialize_list(vm, node, shadow); break;
  /* type declarations (compatible with expression dispatch) */
  case CUBEC_NODE_DECLARATION_ARRAY:
    result = run_declaration_array(vm, node, shadow); break;
  case CUBEC_NODE_DECLARATION_POINTER:
    result = run_declaration_pointer(vm, node, shadow); break;
  case CUBEC_NODE_DECLARATION_SLICE:
    result = run_declaration_slice(vm, node, shadow); break;
  case CUBEC_NODE_DECLARATION_QUALIFIER:
    result = run_declaration_qualifier(vm, node, shadow); break;
  default:
    result = create_exception_value(vm,
                                    "run_expression: unsupported node kind %d",
                                    node->kind);
    break;
  }

  /* script-mode guard: a shadow value in script context means a runtime
   * variable (TDZ / compile-time-only name) was reached at runtime. This
   * is a compile error — the program references a value that has no
   * runtime data. Let errors, interrupts, and void pass through unchanged:
   * errors/interrupts are never shadow, and void is a legitimate runtime
   * "no value" (e.g. assignment result), not a compile-time placeholder. */
  if (!shadow && value_is_shadow(result) &&
      !value_is_error(result) && !value_is_interrupt(result) &&
      type_get_kind(value_get_type(result)) != TYPE_KIND_VOID)
    return create_exception_value(vm,
                                  "run: cannot use compile-time value of type '%s' at runtime",
                                  type_get_name(value_get_type(result)));

  return result;
}

/* ---- statement dispatcher ---- */

value_t run_statement(vm_t vm, node_t node, bool shadow) {
  if (!node) return create_void_value(vm);
  switch (node->kind) {
  case CUBEC_NODE_STATEMENT_EXPRESSION:
    return run_statement_expression(vm, node, shadow);
  case CUBEC_NODE_STATEMENT_BLOCK:
    return run_statement_block(vm, node, shadow);
  case CUBEC_NODE_STATEMENT_EMPTY:
    return create_void_value(vm);
  default:
    if (shadow) {
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           node->location,
                           "run_statement: unsupported statement kind %d",
                           node->kind);
      return create_void_value(vm);
    }
    return create_exception_value(vm,
                                  "run_statement: unsupported statement kind %d",
                                  node->kind);
  }
}
