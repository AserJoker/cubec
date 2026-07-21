/**
 * @file builtin_panic.c
 * @brief Panic builtin: panic(msg: str): void
 *
 * Aborts comptime evaluation with an error message.
 * Future codegen will emit inline abort code ("一体两面").
 */

#include "engine/builtin_panic.h"
#include "engine/comptime_eval_internal.h"
#include "engine/diagnostic.h"
#include "engine/type_hash.h"
#include "cubec/expression_call.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"

/* ===== panic eval callback ===== */

struct comptime_value *builtin_panic_eval(struct comptime_eval *eval,
                                          struct checker *ctx, node_t node,
                                          struct builtin_entry *be) {
  (void)be;
  cubec_expression_call_t call = (cubec_expression_call_t)node;
  size_t acount = call->arguments ? vec_get_size(call->arguments) : 0;

  if (acount < 1) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "panic() requires 1 argument (message: str)");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  comptime_value_t msg_val =
      _comptime_eval_expr(eval, ctx, (node_t)vec_get(call->arguments, 0));
  if (!msg_val || msg_val->kind == COMPTIME_VALUE_ERROR) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "panic");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  const char *msg = NULL;
  if (msg_val->kind == COMPTIME_VALUE_STRING) {
    msg = comptime_value_get_string(msg_val);
  }

  if (msg) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "panic: %s", msg);
  } else {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "panic: <non-string argument>");
  }
  ctx->error_count++;
  return _eval_error_val(eval);
}

/* ===== init ===== */

void builtin_table_init_panic(builtin_table_t table, struct checker *ctx) {
  /* builtin func panic(msg: str): void */
  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  vec_push(params, ctx->builtin_str);
  semantic_type_t panic_type = semantic_type_create_function(
      ctx->allocator, ctx->builtin_void, params, false);
  type_hash_ensure(panic_type);
  vec_push(ctx->all_types, panic_type);
  builtin_table_register(table, "panic", panic_type, builtin_panic_eval);
}
