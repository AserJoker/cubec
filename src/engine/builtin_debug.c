/**
 * @file builtin_debug.c
 * @brief Debug-related builtin functions: assert.
 */

#include "engine/builtin_debug.h"
#include "engine/comptime_eval_internal.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "engine/diagnostic.h"
#include "cubec/expression_call.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"

/* ===== assert eval callback ===== */

struct comptime_value *builtin_assert_eval(struct comptime_eval *eval,
                                         struct checker *ctx, node_t node,
                                         struct builtin_entry *be) {
  (void)be;

  /* assert() is only allowed inside test blocks */
  if (!eval->in_test_block) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "assert can only be used inside test blocks");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  cubec_expression_call_t call = (cubec_expression_call_t)node;
  size_t acount = call->arguments ? vec_get_size(call->arguments) : 0;
  if (acount < 1) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "assert() requires at least 1 argument");
    ctx->error_count++;
    return _eval_error_val(eval);
  }
  comptime_value_t cond =
      _comptime_eval_expr(eval, ctx, (node_t)vec_get(call->arguments, 0));
  if (_val_is_error(cond))
    return _eval_error_val(eval);

  if (!comptime_value_is_truthy(cond)) {
    const char *msg = NULL;
    if (acount >= 2) {
      comptime_value_t msg_val =
          _comptime_eval_expr(eval, ctx, (node_t)vec_get(call->arguments, 1));
      if (msg_val && msg_val->kind == COMPTIME_VALUE_STRING)
        msg = comptime_value_get_string(msg_val);
    }
    if (msg) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           node->location,
                           "assertion failed: %s", msg);
    } else {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           node->location,
                           "assertion failed");
    }
    ctx->error_count++;
    /* In test block: don't abort — continue executing remaining statements.
       Outside test block: this shouldn't happen (checker rejects it). */
    return _eval_temp(eval, comptime_value_create_nil(eval->allocator, NULL));
  }
  return _eval_temp(eval, comptime_value_create_nil(eval->allocator, NULL));
}

/* ===== init ===== */

void builtin_table_init_debug(builtin_table_t table, struct checker *ctx) {
  /* assert(condition: bool): void */
  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  vec_push(params, ctx->builtin_bool);
  semantic_type_t assert_type = semantic_type_create_function(
      ctx->allocator, ctx->builtin_void, params, false);
  type_hash_ensure(assert_type);
  vec_push(ctx->all_types, assert_type);
  builtin_table_register(table, "assert", assert_type, builtin_assert_eval);
}
