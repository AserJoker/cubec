/**
 * @file builtin_union.c
 * @brief Union builtin function: func unionIs[T,K](obj:K):bool
 *
 * Checks whether the active variant of a tagged union value matches type T.
 * The tag is stored in the composite data buffer at offset 0 (uint64_t),
 * containing the type hash of the active variant.
 */

#include "engine/builtin_union.h"
#include "engine/comptime_eval_internal.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "engine/diagnostic.h"
#include "engine/resolver.h"
#include "engine/checker_type_util.h"
#include "cubec/expression_call.h"
#include "cubec/expression_generic_instantiation.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"
#include <string.h>

/* ===== unionIs eval callback ===== */

struct comptime_value *builtin_unionis_eval(struct comptime_eval *eval,
                                            struct checker *ctx, node_t node,
                                            struct builtin_entry *be) {
  (void)be;
  cubec_expression_call_t call = (cubec_expression_call_t)node;
  size_t acount = call->arguments ? vec_get_size(call->arguments) : 0;
  if (acount < 1) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "unionIs() requires 1 argument");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  /* Evaluate the argument */
  comptime_value_t obj_val = _comptime_eval_expr(eval, ctx,
      (node_t)vec_get(call->arguments, 0));
  if (_val_is_error(obj_val))
    return _eval_error_val(eval);

  /* Verify the argument is a tagged union composite */
  if (obj_val->kind != COMPTIME_VALUE_COMPOSITE) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "unionIs requires a union value");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  semantic_type_t obj_type = obj_val->type;
  semantic_type_t obj_unq = semantic_type_strip_qualifier(obj_type);
  bool is_union = false;
  if (obj_unq->impl->kind == TYPE_UNION) {
    is_union = true;
  } else if (obj_unq->impl->kind == TYPE_GENERIC_INSTANCE) {
    semantic_type_t base = obj_unq->impl->generic_instance.generic_template;
    if (base && base->impl->kind == TYPE_UNION) is_union = true;
  }

  if (!is_union) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "unionIs requires a union type, got '%s'",
                         semantic_type_get_name(obj_type)
                             ? semantic_type_get_name(obj_type) : "?");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  /* Get target type T from the generic instantiation's first type argument */
  semantic_type_t target_type = NULL;
  {
    node_t callee_node = call->callee;
    if (callee_node &&
        callee_node->kind == CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION) {
      cubec_expression_generic_instantiation_t gi =
          (cubec_expression_generic_instantiation_t)callee_node;
      if (gi->arguments && vec_get_size(gi->arguments) >= 1) {
        node_t t_expr = (node_t)vec_get(gi->arguments, 0);
        target_type = resolver_resolve_type(ctx, t_expr);
        if (!target_type || target_type->impl->kind == TYPE_ERROR) {
          vec_t gp = NULL;
          const char *name = _checker_ident_str(gi->callee);
          struct symbol *sym = name ? scope_lookup(ctx->current_scope, name) : NULL;
          if (sym && sym->kind == SYMBOL_FUNCTION && sym->function.generic_params) {
            gp = sym->function.generic_params;
          }
          vec_t resolved_args = _resolve_generic_type_args(ctx, gi->arguments, gp);
          if (resolved_args && vec_get_size(resolved_args) >= 1) {
            target_type = (semantic_type_t)vec_get(resolved_args, 0);
          }
          if (resolved_args)
            allocator_free(ctx->allocator, &resolved_args);
        }
      }
    }
  }

  if (!target_type) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "unionIs: could not resolve target type");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  /* Compare the union tag with the target type hash */
  type_hash_ensure(target_type);
  uint64_t obj_tag = comptime_value_get_union_tag(obj_val);
  bool result = (obj_tag == target_type->impl->hash);

  return _eval_temp(eval, comptime_value_create_bool(eval->allocator, result,
      ctx->builtin_bool));
}

/* ===== init ===== */

void builtin_table_init_union(builtin_table_t table, struct checker *ctx) {
  /* builtin func unionIs[T,K](obj:K):bool */
  semantic_type_t t_param = semantic_type_create_generic_param(
      ctx->allocator, "T", NULL, false);
  type_hash_ensure(t_param);
  vec_push(ctx->all_types, t_param);

  semantic_type_t k_param = semantic_type_create_generic_param(
      ctx->allocator, "K", NULL, false);
  type_hash_ensure(k_param);
  vec_push(ctx->all_types, k_param);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  vec_push(params, k_param);

  semantic_type_t unionis_type = semantic_type_create_function(
      ctx->allocator, ctx->builtin_bool, params, false);
  type_hash_ensure(unionis_type);
  vec_push(ctx->all_types, unionis_type);
  builtin_table_register(table, "unionIs", unionis_type, builtin_unionis_eval);
}
