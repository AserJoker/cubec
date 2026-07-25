/**
 * @file builtin_collection.c
 * @brief Collection-related builtin functions: length.
 */

#include "engine/builtin_collection.h"
#include "engine/comptime_eval_internal.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "engine/diagnostic.h"
#include "cubec/expression_call.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"
#include <string.h>

/* ===== length eval callback ===== */

struct comptime_value *builtin_length_eval(struct comptime_eval *eval,
                                         struct context *ctx, node_t node,
                                         struct builtin_entry *be) {
  (void)be;
  cubec_expression_call_t call = (cubec_expression_call_t)node;
  size_t acount = call->arguments ? vec_get_size(call->arguments) : 0;
  if (acount < 1) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "length() requires at least 1 argument");
    ctx->error_count++;
    return _eval_error_val(eval);
  }
  comptime_value_t arg =
      _comptime_eval_expr(eval, ctx, (node_t)vec_get(call->arguments, 0));
  if (_val_is_error(arg))
    return _eval_error_val(eval);

  uint64_t len = 0;
  if (arg->type) {
    if (arg->type->impl->kind == TYPE_ARRAY) {
      len = arg->type->impl->array.length;
    } else if (arg->type->impl->kind == TYPE_SLICE) {
      /* slice = { data, start, length }; length is at offset 2*ptr_size */
      if (arg->kind == COMPTIME_VALUE_COMPOSITE && arg->composite.data) {
        const size_t ptr_size = 8; /* matches type_layout_compute default */
        memcpy(&len, arg->composite.data + 2 * ptr_size, ptr_size);
      }
    } else if (arg->type->impl->kind == TYPE_TUPLE &&
               arg->type->impl->tuple.fields) {
      len = vec_get_size(arg->type->impl->tuple.fields);
    } else if (arg->type->impl->kind == TYPE_STR) {
      const char *s = comptime_value_get_string(arg);
      len = s ? (uint64_t)strlen(s) : 0;
    } else {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           node->location,
                           "length() requires an array, slice, or tuple argument");
      ctx->error_count++;
      return _eval_error_val(eval);
    }
  }
  return _eval_temp(eval, comptime_value_create_int(
      eval->allocator, (int64_t)len, len, 64, false, ctx->builtin_u64));
}

/* ===== init ===== */

void builtin_table_init_collection(builtin_table_t table, struct context *ctx) {
  /* builtin func length[T](list: T): u64 */
  semantic_type_t t_param = semantic_type_create_generic_param(
      ctx->allocator, "T", NULL, false);
  type_hash_ensure(t_param);
  vec_push(ctx->all_types, t_param);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  vec_push(params, t_param);
  semantic_type_t length_type = semantic_type_create_function(
      ctx->allocator, ctx->builtin_u64, params, false);
  type_hash_ensure(length_type);
  vec_push(ctx->all_types, length_type);
  builtin_table_register(table, "length", length_type, builtin_length_eval);
}
