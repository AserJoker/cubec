#include "engine/resolver.h"
#include "engine/resolver_types.h"
#include "engine/checker_type_util.h"
#include "engine/type_hash.h"
#include "cubec/node.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_wildcard.h"
#include "cubec/expression_spread.h"

/* ===== resolver_resolve_type (dispatcher) ===== */

semantic_type_t resolver_resolve_type(context_t ctx, node_t node) {
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

  case CUBEC_NODE_EXPRESSION_TYPE_TUPLE:
    return _resolve_type_tuple(ctx, node);

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

  case CUBEC_NODE_EXPRESSION_SPREAD: {
    /* Spread in type context: ...Args resolves to the inner type (TYPE_GENERIC_PACK) */
    cubec_expression_spread_t spread = (cubec_expression_spread_t)node;
    if (spread->value)
      return resolver_resolve_type(ctx, spread->value);
    return ctx->error_type;
  }

  case CUBEC_NODE_EXPRESSION_WILDCARD: {
    /* Wildcard: is_tuple=true means <?> (any tuple), false means standalone ? */
    cubec_expression_wildcard_t wnode = (cubec_expression_wildcard_t)node;
    semantic_type_t wt = semantic_type_create_wildcard(ctx->allocator, wnode->is_tuple);
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
      if (template_type->impl->kind == TYPE_GENERIC_INSTANCE) {
        /* Resolve args just for validation, then return the pre-existing instance */
        strmap_t dummy = _resolve_generic_type_bindings_pack(ctx, gi->arguments, sym->type.generic_params);
        allocator_free(ctx->allocator, &dummy);
        return template_type;
      }

      strmap_t type_bindings = _resolve_generic_type_bindings_pack(ctx, gi->arguments, sym->type.generic_params);
      if (strmap_get_size(type_bindings) == 0 && vec_get_size(gi->arguments) > 0) {
        allocator_free(ctx->allocator, &type_bindings);
        return ctx->error_type;
      }
      _check_generic_param_constraints(ctx, sym->type.generic_params, type_bindings, node);
      return _instantiate_type(ctx, template_type, type_bindings, node);
    }

    if (sym && sym->kind == SYMBOL_GENERIC_PARAM) {
      /* Generic param with index arg: Args[N] → TYPE_PACK_INDEX.
         This occurs in builtin function return types like getTupleItem[N, ...Args](tuple): Args[N]. */
      if (sym->generic_param.is_rest) {
        /* Args[N]: the argument should be a value generic param (the index).
           Resolve the index argument to get its value. */
        vec_t type_args = _resolve_generic_type_args(ctx, gi->arguments, NULL);
        if (type_args && vec_get_size(type_args) >= 1) {
          semantic_type_t idx_type = (semantic_type_t)vec_get(type_args, 0);
          /* Derive the index param name from the first generic instantiation argument */
          const char *index_param_name = NULL;
          if (gi->arguments && vec_get_size(gi->arguments) >= 1) {
            node_t idx_expr = (node_t)vec_get(gi->arguments, 0);
            if (idx_expr && idx_expr->kind == CUBEC_NODE_LITERAL_IDENTIFIER)
              index_param_name = _resolver_ident_str(idx_expr);
          }
          if (idx_type && idx_type->impl->kind == TYPE_GENERIC_VALUE) {
            semantic_type_t pack_idx_type = semantic_type_create_pack_index(
                ctx->allocator, name, index_param_name);
            type_hash_ensure(pack_idx_type);
            vec_push(ctx->all_types, pack_idx_type);
            allocator_free(ctx->allocator, &type_args);
            return pack_idx_type;
          }
          /* Index is a generic param (not yet resolved to value) — create pack_index with placeholder */
          if (idx_type && idx_type->impl->kind == TYPE_GENERIC_PARAM) {
            if (!index_param_name && idx_type->impl->generic_param.name)
              index_param_name = idx_type->impl->generic_param.name;
            semantic_type_t pack_idx_type = semantic_type_create_pack_index(
                ctx->allocator, name, index_param_name);
            type_hash_ensure(pack_idx_type);
            vec_push(ctx->all_types, pack_idx_type);
            allocator_free(ctx->allocator, &type_args);
            return pack_idx_type;
          }
        }
        if (type_args) allocator_free(ctx->allocator, &type_args);
      }
      /* Non-rest generic param with args: just return the param type */
      return _resolve_type_identifier(ctx, gi->callee);
    }

    if (sym && sym->kind == SYMBOL_FUNCTION && sym->function.type) {
      strmap_t type_bindings_fn = _resolve_generic_type_bindings_pack(ctx, gi->arguments, sym->function.generic_params);
      if (strmap_get_size(type_bindings_fn) == 0 && vec_get_size(gi->arguments) > 0) {
        allocator_free(ctx->allocator, &type_bindings_fn);
        return ctx->error_type;
      }
      _check_generic_param_constraints(ctx, sym->function.generic_params, type_bindings_fn, node);
      semantic_type_t result = _instantiate_function(ctx, sym, type_bindings_fn, node);
      allocator_free(ctx->allocator, &type_bindings_fn);
      return result;
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
