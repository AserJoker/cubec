/**
 * @file builtin_slice.c
 * @brief Slice-related builtin functions: makeSlice.
 */

#include "engine/builtin_slice.h"
#include "engine/comptime_eval_internal.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
#include "engine/resolver.h"
#include "cubec/expression_call.h"
#include "cubec/expression_generic_instantiation.h"
#include <string.h>

/* ===== makeSlice eval callback ===== */

struct comptime_value *builtin_makeSlice_eval(struct comptime_eval *eval,
                                               struct context *ctx, node_t node,
                                               struct builtin_entry *be) {
  (void)be;
  cubec_expression_call_t call = (cubec_expression_call_t)node;
  size_t acount = call->arguments ? vec_get_size(call->arguments) : 0;
  if (acount < 3) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "makeSlice() requires 3 arguments (pointer, start, len)");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  /* Evaluate arguments */
  comptime_value_t ptr_val =
      _comptime_eval_expr(eval, ctx, (node_t)vec_get(call->arguments, 0));
  if (_val_is_error(ptr_val)) return _eval_error_val(eval);

  comptime_value_t start_val =
      _comptime_eval_expr(eval, ctx, (node_t)vec_get(call->arguments, 1));
  if (_val_is_error(start_val)) return _eval_error_val(eval);

  comptime_value_t len_val =
      _comptime_eval_expr(eval, ctx, (node_t)vec_get(call->arguments, 2));
  if (_val_is_error(len_val)) return _eval_error_val(eval);

  /* Resolve element type T from generic instantiation */
  semantic_type_t elem_type = NULL;
  {
    node_t callee_node = call->callee;
    if (callee_node &&
        callee_node->kind == CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION) {
      cubec_expression_generic_instantiation_t gi =
          (cubec_expression_generic_instantiation_t)callee_node;
      if (gi->arguments && vec_get_size(gi->arguments) >= 1) {
        node_t t_expr = (node_t)vec_get(gi->arguments, 0);
        elem_type = resolver_resolve_type(ctx, t_expr);
        if (!elem_type || elem_type->impl->kind == TYPE_ERROR) {
          /* Fallback: resolve type args using the generic param list */
          vec_t gp = NULL;
          const char *name = _checker_ident_str(gi->callee);
          struct symbol *sym = name ? scope_lookup(ctx->current_scope, name) : NULL;
          if (sym && sym->kind == SYMBOL_FUNCTION && sym->function.generic_params)
            gp = sym->function.generic_params;
          vec_t resolved_args = _resolve_generic_type_args(ctx, gi->arguments, gp);
          if (resolved_args && vec_get_size(resolved_args) >= 1)
            elem_type = (semantic_type_t)vec_get(resolved_args, 0);
          if (resolved_args)
            allocator_free(ctx->allocator, &resolved_args);
        }
      }
    }
  }

  /* Fallback: derive from pointer element type */
  if (!elem_type && ptr_val->type) {
    semantic_type_t ptr_unq = semantic_type_strip_qualifier(ptr_val->type);
    if (ptr_unq->impl->kind == TYPE_POINTER)
      elem_type = ptr_unq->impl->pointer.pointee;
  }

  if (!elem_type) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "makeSlice: could not resolve element type");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  /* Build slice type []T */
  semantic_type_t slice_type = semantic_type_create_slice(ctx->allocator, elem_type);
  type_hash_ensure(slice_type);
  type_layout_compute(slice_type, 8);
  vec_push(ctx->all_types, slice_type);

  size_t data_size = slice_type->impl->size;
  /* slice layout: { T* data; size_t start; size_t length; }
     On 64-bit: data(8) + start(8) + length(8) = 24 bytes
     ptr_size matches the value used in type_layout_compute */
  const size_t ptr_size = 8;

  /* Create composite value for slice */
  comptime_value_t result = comptime_value_create_composite(
      eval->allocator, slice_type, elem_type, data_size);
  if (!result) return _eval_error_val(eval);

  /* Write data pointer at offset 0 */
  comptime_value_write_field(result, 0, ptr_val->type, ptr_val, eval->allocator);

  /* Write start at offset ptr_size */
  semantic_type_t u64_type = ctx->builtin_u64;
  comptime_value_write_field(result, ptr_size, u64_type, start_val, eval->allocator);

  /* Write length at offset 2 * ptr_size */
  comptime_value_write_field(result, 2 * ptr_size, u64_type, len_val, eval->allocator);

  return _eval_temp(eval, result);
}

/* ===== init ===== */

void builtin_table_init_slice(builtin_table_t table, struct context *ctx) {
  /* builtin func makeSlice[T](pointer: *T, start: u64, len: u64): []T */
  semantic_type_t t_param = semantic_type_create_generic_param(
      ctx->allocator, "T", NULL, false);
  type_hash_ensure(t_param);
  vec_push(ctx->all_types, t_param);

  /* Build parameter types: *T, u64, u64 */
  semantic_type_t ptr_t = semantic_type_create_pointer(ctx->allocator, t_param);
  type_hash_ensure(ptr_t);
  vec_push(ctx->all_types, ptr_t);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  vec_push(params, ptr_t);
  vec_push(params, ctx->builtin_u64);
  vec_push(params, ctx->builtin_u64);

  /* Return type: []T */
  semantic_type_t slice_t = semantic_type_create_slice(ctx->allocator, t_param);
  type_hash_ensure(slice_t);
  vec_push(ctx->all_types, slice_t);

  semantic_type_t makeslice_type = semantic_type_create_function(
      ctx->allocator, slice_t, params, false);
  type_hash_ensure(makeslice_type);
  vec_push(ctx->all_types, makeslice_type);
  builtin_table_register(table, "makeSlice", makeslice_type, builtin_makeSlice_eval);
}
