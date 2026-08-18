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
  switch (node->kind) {
  /* literals */
  case CUBEC_NODE_LITERAL_NUMERIC:
    return run_literal_numeric(vm, node, shadow);
  case CUBEC_NODE_LITERAL_STRING:
    return run_literal_string(vm, node, shadow);
  case CUBEC_NODE_LITERAL_CHAR:
    return run_literal_char(vm, node, shadow);
  case CUBEC_NODE_LITERAL_IDENTIFIER:
    return run_literal_identifier(vm, node, shadow);
  case CUBEC_NODE_LITERAL_NIL:
    return run_literal_nil(vm, node, shadow);
  case CUBEC_NODE_LITERAL_BOOL:
    return run_literal_bool(vm, node, shadow);
  case CUBEC_NODE_LITERAL_UNDEFINED:
    return run_literal_undefined(vm, node, shadow);
  /* expressions */
  case CUBEC_NODE_EXPRESSION_BINARY:
    return run_expression_binary(vm, node, shadow);
  case CUBEC_NODE_EXPRESSION_ASSIGNMENT:
    return run_expression_assignment(vm, node, shadow);
  case CUBEC_NODE_EXPRESSION_DEREF:
    return run_expression_deref(vm, node, shadow);
  case CUBEC_NODE_EXPRESSION_ADDR:
    return run_expression_addr(vm, node, shadow);
  case CUBEC_NODE_EXPRESSION_MEMBER:
    return run_expression_member(vm, node, shadow);
  case CUBEC_NODE_EXPRESSION_CALL:
    return run_expression_call(vm, node, shadow);
  case CUBEC_NODE_EXPRESSION_GROUP:
    return run_expression_group(vm, node, shadow);
  case CUBEC_NODE_EXPRESSION_SUBSCRIPT:
    /* subscript covers the unified [...] syntax; semantic analysis
     * decides whether host[args] is a subscript or generic instantiation,
     * but at run time runtime values use subscript (value_get_item). */
    return run_expression_subscript(vm, node, shadow);
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS:
    return run_expression_namespace_access(vm, node, shadow);
  case CUBEC_NODE_EXPRESSION_INITIALIZE_LIST:
    return run_expression_initialize_list(vm, node, shadow);
  /* type declarations (compatible with expression dispatch) */
  case CUBEC_NODE_DECLARATION_ARRAY:
    return run_declaration_array(vm, node, shadow);
  case CUBEC_NODE_DECLARATION_POINTER:
    return run_declaration_pointer(vm, node, shadow);
  case CUBEC_NODE_DECLARATION_SLICE:
    return run_declaration_slice(vm, node, shadow);
  case CUBEC_NODE_DECLARATION_QUALIFIER:
    return run_declaration_qualifier(vm, node, shadow);
  default:
    return create_exception_value(vm,
                                  "run_expression: unsupported node kind %d",
                                  node->kind);
  }
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
