#include "engine/resolver.h"
#include "engine/resolver_types.h"
#include "engine/checker_type_util.h"
#include "engine/symbol.h"
#include "engine/scope.h"
#include "engine/diagnostic.h"
#include "engine/type_hash.h"
#include "cubec/node.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_spread.h"
#include "cubec/literal_identifier.h"
#include "cubec/generic_param.h"

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

  case CUBEC_NODE_EXPRESSION_SPREAD: {
    /* Spread in type context: ...Args resolves to the inner type (TYPE_GENERIC_PACK) */
    cubec_expression_spread_t spread = (cubec_expression_spread_t)node;
    if (spread->value)
      return resolver_resolve_type(ctx, spread->value);
    return ctx->error_type;
  }

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
      vec_t type_args = _resolve_generic_type_args(ctx, gi->arguments, sym->type.generic_params);
      if (!type_args) return ctx->error_type;
      if (template_type->impl->kind == TYPE_GENERIC_INSTANCE) {
        allocator_free(ctx->allocator, &type_args);
        return template_type;
      }

      /* Coalesce excess type args into packs for generic types with rest params */
      vec_t gp = sym->type.generic_params;
      size_t gcount = gp ? vec_get_size(gp) : 0;
      size_t tacount = type_args ? vec_get_size(type_args) : 0;
      /* Find the pack parameter position */
      size_t pack_idx = gcount; /* default: no pack */
      for (size_t i = 0; i < gcount; i++) {
        cubec_generic_param_t gp_node = (cubec_generic_param_t)(void *)vec_get(gp, i);
        if (gp_node && gp_node->is_rest) {
          pack_idx = i;
          break;
        }
      }
      /* If there's a pack param and type_args include values at or beyond pack_idx,
         coalesce them into a TYPE_GENERIC_PACK */
      if (pack_idx < gcount && tacount >= pack_idx) {
        vec_init_t vi = {.auto_dispose = false};
        vec_t new_type_args = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
        for (size_t i = 0; i < pack_idx; i++)
          vec_push(new_type_args, vec_get(type_args, i));
        const char *pack_name = NULL;
        cubec_generic_param_t pack_gp = (cubec_generic_param_t)(void *)vec_get(gp, pack_idx);
        if (pack_gp) {
          const char *raw = _checker_ident_str(pack_gp->name);
          if (raw) pack_name = raw;
        }
        semantic_type_t pack_type = semantic_type_create_generic_pack(
            ctx->allocator, pack_name, pack_idx);
        for (size_t i = pack_idx; i < tacount; i++) {
          semantic_type_t ta = (semantic_type_t)vec_get(type_args, i);
          vec_push(pack_type->impl->generic_pack.expanded_types, ta);
        }
        type_hash_ensure(pack_type);
        vec_push(ctx->all_types, pack_type);
        vec_push(new_type_args, pack_type);
        allocator_free(ctx->allocator, &type_args);
        type_args = new_type_args;
      }

      _check_generic_param_constraints(ctx, sym->type.generic_params, type_args, node);
      return _instantiate_type(ctx, template_type, type_args, node);
    }

    if (sym && sym->kind == SYMBOL_FUNCTION && sym->function.type) {
      vec_t type_args = _resolve_generic_type_args(ctx, gi->arguments, sym->function.generic_params);
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
