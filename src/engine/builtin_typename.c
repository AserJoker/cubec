/**
 * @file builtin_typename.c
 * @brief Typename builtin function: builtin func typename[T](): str
 *
 * Returns the readable name of type T at compile time.
 */

#include "engine/builtin_typename.h"
#include "engine/comptime_eval_internal.h"
#include "engine/diagnostic.h"
#include "engine/resolver.h"
#include "engine/type_hash.h"
#include "engine/checker_type_util.h"
#include "cubec/expression_call.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/literal_identifier.h"
#include "core/allocator.h"
#include "core/vec.h"

/* ===== typename eval callback ===== */

struct comptime_value *builtin_typename_eval(struct comptime_eval *eval,
                                              struct checker *ctx, node_t node,
                                              struct builtin_entry *be) {
  (void)be;
  cubec_expression_call_t call = (cubec_expression_call_t)node;

  /* Get type T from the generic instantiation's first type argument */
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
                         "typename: could not resolve type argument");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  const char *type_name = target_type->name ? target_type->name : "<anonymous>";
  return _eval_temp(eval, comptime_value_create_string(
      eval->allocator, type_name, ctx->builtin_str));
}

/* ===== init ===== */

void builtin_table_init_typename(builtin_table_t table, struct checker *ctx) {
  /* builtin func typename[T](): str */
  semantic_type_t t_param = semantic_type_create_generic_param(
      ctx->allocator, "T", NULL, false);
  type_hash_ensure(t_param);
  vec_push(ctx->all_types, t_param);

  /* No runtime parameters — empty vec */
  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  semantic_type_t typename_type = semantic_type_create_function(
      ctx->allocator, ctx->builtin_str, params, false);
  type_hash_ensure(typename_type);
  vec_push(ctx->all_types, typename_type);
  builtin_table_register(table, "typename", typename_type, builtin_typename_eval);
}
