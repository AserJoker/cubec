#include "engine/resolver.h"
#include "engine/resolver_types.h"
#include "engine/checker_type_util.h"
#include "engine/symbol.h"
#include "engine/scope.h"
#include "engine/diagnostic.h"
#include "cubec/node.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/literal_identifier.h"

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

  case CUBEC_NODE_EXPRESSION_WILDCARD: {
    semantic_type_t wt = semantic_type_create_wildcard(ctx->allocator);
    vec_push(ctx->all_types, wt);
    return wt;
  }

  case CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION: {
    /* Resolve generic instantiation in type position: Vec[i32] etc. */
    cubec_expression_generic_instantiation_t gi =
        (cubec_expression_generic_instantiation_t)node;
    const char *name = _checker_ident_str(gi->callee);
    struct symbol *sym = name ? scope_lookup(ctx->current_scope, name) : NULL;

    if (sym && sym->kind == SYMBOL_TYPE && sym->type.type) {
      semantic_type_t template_type = sym->type.type;
      vec_t type_args = _resolve_generic_type_args(ctx, gi->arguments);
      if (!type_args) return ctx->error_type;
      if (template_type->impl->kind == TYPE_GENERIC_INSTANCE) {
        allocator_free(ctx->allocator, &type_args);
        return template_type;
      }
      _check_generic_param_constraints(ctx, sym->type.generic_params, type_args, node);
      return _instantiate_type(ctx, template_type, type_args, node);
    }

    if (sym && sym->kind == SYMBOL_FUNCTION && sym->function.type) {
      vec_t type_args = _resolve_generic_type_args(ctx, gi->arguments);
      if (!type_args) return ctx->error_type;
      _check_generic_param_constraints(ctx, sym->function.generic_params, type_args, node);
      return _instantiate_function(ctx, sym, type_args, node);
    }

    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "'%s' is not a generic type or function", name ? name : "<unknown>");
    ctx->error_count++;
    return ctx->error_type;
  }

  default:
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "invalid type expression (node kind %d)", node->kind);
    ctx->error_count++;
    return ctx->error_type;
  }
}
