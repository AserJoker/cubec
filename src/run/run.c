#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/diagnostic.h"
#include "engine/value.h"
#include "cubec/node.h"

/* ---- expression dispatcher ---- */

value_t run_expression(context_t ctx, node_t node, bool shadow) {
  if (!node) return create_void_value(ctx->vm);
  switch (node->kind) {
  /* literals */
  case CUBEC_NODE_LITERAL_NUMERIC:
    return run_literal_numeric(ctx, node, shadow);
  case CUBEC_NODE_LITERAL_STRING:
    return run_literal_string(ctx, node, shadow);
  case CUBEC_NODE_LITERAL_CHAR:
    return run_literal_char(ctx, node, shadow);
  case CUBEC_NODE_LITERAL_IDENTIFIER:
    return run_literal_identifier(ctx, node, shadow);
  case CUBEC_NODE_LITERAL_NIL:
    return run_literal_nil(ctx, node, shadow);
  case CUBEC_NODE_LITERAL_BOOL:
    return run_literal_bool(ctx, node, shadow);
  case CUBEC_NODE_LITERAL_UNDEFINED:
    return run_literal_undefined(ctx, node, shadow);
  /* expressions */
  case CUBEC_NODE_EXPRESSION_BINARY:
    return run_expression_binary(ctx, node, shadow);
  case CUBEC_NODE_EXPRESSION_ASSIGNMENT:
    return run_expression_assignment(ctx, node, shadow);
  case CUBEC_NODE_EXPRESSION_DEREF:
    return run_expression_deref(ctx, node, shadow);
  case CUBEC_NODE_EXPRESSION_ADDR:
    return run_expression_addr(ctx, node, shadow);
  case CUBEC_NODE_EXPRESSION_MEMBER:
    return run_expression_member(ctx, node, shadow);
  case CUBEC_NODE_EXPRESSION_CALL:
    return run_expression_call(ctx, node, shadow);
  case CUBEC_NODE_EXPRESSION_GROUP:
    return run_expression_group(ctx, node, shadow);
  case CUBEC_NODE_EXPRESSION_SUBSCRIPT:
    /* subscript covers the unified [...] syntax; semantic analysis
     * decides whether host[args] is a subscript or generic instantiation,
     * but at run time runtime values use subscript (value_get_item). */
    return run_expression_subscript(ctx, node, shadow);
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS:
    return run_expression_namespace_access(ctx, node, shadow);
  default:
    return create_exception_value(ctx->vm,
                                  "run_expression: unsupported node kind %d",
                                  node->kind);
  }
}

/* ---- statement dispatcher ---- */

value_t run_statement(context_t ctx, node_t node, bool shadow) {
  if (!node) return create_void_value(ctx->vm);
  switch (node->kind) {
  case CUBEC_NODE_STATEMENT_EXPRESSION:
    return run_statement_expression(ctx, node, shadow);
  case CUBEC_NODE_STATEMENT_BLOCK:
    return run_statement_block(ctx, node, shadow);
  case CUBEC_NODE_STATEMENT_EMPTY:
    return create_void_value(ctx->vm);
  default:
    if (shadow) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           node->location,
                           "run_statement: unsupported statement kind %d",
                           node->kind);
      return create_void_value(ctx->vm);
    }
    return create_exception_value(ctx->vm,
                                  "run_statement: unsupported statement kind %d",
                                  node->kind);
  }
}
