#include "engine/resolver.h"
#include "engine/resolver_types.h"
#include "cubec/node.h"

/* ===== resolver_resolve_type (dispatcher) ===== */

semantic_type_t resolver_resolve_type(checker_t ctx, node_t node) {
  if (!node) return ctx->builtin_void;

  switch (node->kind) {

  case CUBEC_NODE_LITERAL_IDENTIFIER:
    return _resolve_type_identifier(ctx, node);

  case CUBEC_NODE_DECLARATION_POINTER:
    return _resolve_type_pointer(ctx, node);

  case CUBEC_NODE_DECLARATION_SLICE:
    return _resolve_type_slice(ctx, node);

  case CUBEC_NODE_DECLARATION_ARRAY:
    return _resolve_type_array(ctx, node);

  case CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER:
    return _resolve_type_qualifier(ctx, node);

  case CUBEC_NODE_EXPRESSION_TYPE_STRUCT:
    return _resolve_type_struct(ctx, node);

  case CUBEC_NODE_EXPRESSION_TYPE_ENUM:
    return _resolve_type_enum(ctx, node);

  case CUBEC_NODE_EXPRESSION_TYPE_UNION:
    return _resolve_type_union(ctx, node);

  case CUBEC_NODE_EXPRESSION_TYPE_INTERFACE:
    return _resolve_type_interface(ctx, node);

  case CUBEC_NODE_EXPRESSION_TYPE_FUNCTION:
    return _resolve_type_function(ctx, node);

  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS:
    return _resolve_type_namespace_access(ctx, node);

  case CUBEC_NODE_EXPRESSION_TYPEOF:
    return _resolve_type_typeof(ctx, node);

  default:
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "invalid type expression (node kind %d)", node->kind);
    ctx->error_count++;
    return ctx->error_type;
  }
}
