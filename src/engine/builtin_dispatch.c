/**
 * @file builtin_dispatch.c
 * @brief Builtin function eval callbacks — extracted from comptime_eval_expr.c.
 *
 * Each builtin function that requires comptime evaluation registers an eval_call
 * callback here. Adding a new builtin only requires:
 *   1. Implement the callback in this file
 *   2. Register it via builtin_table_register() in builtin.c
 */

#include "engine/builtin_dispatch.h"
#include "engine/comptime_eval_internal.h"
#include "engine/symbol.h"
#include "engine/type_layout.h"
#include "engine/diagnostic.h"
#include "cubec/expression_call.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/literal_numeric.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"

/* ===== assert ===== */

struct comptime_value *builtin_assert_eval(struct comptime_eval *eval,
                                         struct checker *ctx, node_t node,
                                         struct builtin_entry *be) {
  (void)be;
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
  if (!cond || cond->kind == COMPTIME_VALUE_ERROR)
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
    return _eval_error_val(eval);
  }
  return _eval_temp(eval, comptime_value_create_nil(eval->allocator, NULL));
}

/* ===== length ===== */

struct comptime_value *builtin_length_eval(struct comptime_eval *eval,
                                         struct checker *ctx, node_t node,
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
  if (!arg || arg->kind == COMPTIME_VALUE_ERROR)
    return _eval_error_val(eval);

  uint64_t len = 0;
  if (arg->type) {
    if (arg->type->impl->kind == TYPE_ARRAY) {
      len = arg->type->impl->array.length;
    } else if (arg->type->impl->kind == TYPE_GENERIC_INSTANCE &&
               arg->type->impl->generic_instance.fields) {
      /* Tuple: length = number of fields */
      len = vec_get_size(arg->type->impl->generic_instance.fields);
    } else {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           node->location,
                           "length() requires an array or tuple argument");
      ctx->error_count++;
      return _eval_error_val(eval);
    }
  }
  return _eval_temp(eval, comptime_value_create_int(
      eval->allocator, (int64_t)len, len, 64, false, ctx->builtin_u64));
}

/* ===== getTupleItem ===== */

struct comptime_value *builtin_get_eval(struct comptime_eval *eval,
                                      struct checker *ctx, node_t node,
                                      struct builtin_entry *be) {
  (void)be;
  cubec_expression_call_t call = (cubec_expression_call_t)node;
  size_t acount = call->arguments ? vec_get_size(call->arguments) : 0;
  if (acount < 1) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "getTupleItem() requires a tuple argument");
    ctx->error_count++;
    return _eval_error_val(eval);
  }
  comptime_value_t tuple_val =
      _comptime_eval_expr(eval, ctx, (node_t)vec_get(call->arguments, 0));
  if (!tuple_val || tuple_val->kind == COMPTIME_VALUE_ERROR)
    return _eval_error_val(eval);

  /* Get index N from the generic instantiation expression */
  uint64_t idx = 0;
  {
    node_t callee_node = call->callee;
    if (callee_node && callee_node->kind == CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION) {
      cubec_expression_generic_instantiation_t gi =
          (cubec_expression_generic_instantiation_t)callee_node;
      if (gi->arguments && vec_get_size(gi->arguments) >= 1) {
        node_t n_arg = (node_t)vec_get(gi->arguments, 0);
        if (n_arg && n_arg->kind == CUBEC_NODE_LITERAL_NUMERIC) {
          cubec_literal_numeric_t num = (cubec_literal_numeric_t)n_arg;
          idx = strtoull(string_get(num->value), NULL, 10);
        }
      }
    }
  }

  /* Get the Nth field from the tuple composite */
  if (tuple_val->kind != COMPTIME_VALUE_COMPOSITE) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "getTupleItem() requires a tuple argument");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  semantic_type_t tuple_type = tuple_val->type;
  if (!tuple_type || tuple_type->impl->kind != TYPE_GENERIC_INSTANCE ||
      !tuple_type->impl->generic_instance.fields) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "getTupleItem() requires a tuple argument");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  vec_t fields = tuple_type->impl->generic_instance.fields;
  size_t fcount = vec_get_size(fields);
  if (idx >= fcount) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "tuple index %llu out of range", (unsigned long long)idx);
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  struct symbol *f = (struct symbol *)vec_get(fields, (size_t)idx);
  if (!f || !f->field.type) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "tuple field %llu has no type", (unsigned long long)idx);
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  return _eval_temp(eval, comptime_value_read_field(tuple_val, f->field.offset,
                                                    f->field.type, eval->allocator));
}

/* ===== setTupleItem ===== */

struct comptime_value *builtin_set_eval(struct comptime_eval *eval,
                                      struct checker *ctx, node_t node,
                                      struct builtin_entry *be) {
  (void)be;
  cubec_expression_call_t call = (cubec_expression_call_t)node;
  size_t acount = call->arguments ? vec_get_size(call->arguments) : 0;
  if (acount < 2) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "setTupleItem() requires tuple and value arguments");
    ctx->error_count++;
    return _eval_error_val(eval);
  }
  comptime_value_t tuple_val =
      _comptime_eval_expr(eval, ctx, (node_t)vec_get(call->arguments, 0));
  comptime_value_t value_val =
      _comptime_eval_expr(eval, ctx, (node_t)vec_get(call->arguments, 1));
  if (!tuple_val || tuple_val->kind == COMPTIME_VALUE_ERROR ||
      !value_val || value_val->kind == COMPTIME_VALUE_ERROR)
    return _eval_error_val(eval);

  /* Get index N from the generic instantiation expression */
  uint64_t idx = 0;
  {
    node_t callee_node = call->callee;
    if (callee_node && callee_node->kind == CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION) {
      cubec_expression_generic_instantiation_t gi =
          (cubec_expression_generic_instantiation_t)callee_node;
      if (gi->arguments && vec_get_size(gi->arguments) >= 1) {
        node_t n_arg = (node_t)vec_get(gi->arguments, 0);
        if (n_arg && n_arg->kind == CUBEC_NODE_LITERAL_NUMERIC) {
          cubec_literal_numeric_t num = (cubec_literal_numeric_t)n_arg;
          idx = strtoull(string_get(num->value), NULL, 10);
        }
      }
    }
  }

  /* Get the Nth field from the tuple composite */
  if (tuple_val->kind != COMPTIME_VALUE_COMPOSITE) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "set() requires a tuple argument");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  semantic_type_t tuple_type = tuple_val->type;
  if (!tuple_type || tuple_type->impl->kind != TYPE_GENERIC_INSTANCE ||
      !tuple_type->impl->generic_instance.fields) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "set() requires a tuple argument");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  vec_t fields = tuple_type->impl->generic_instance.fields;
  size_t fcount = vec_get_size(fields);
  if (idx >= fcount) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "tuple index %llu out of range", (unsigned long long)idx);
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  struct symbol *f = (struct symbol *)vec_get(fields, (size_t)idx);
  if (!f || !f->name) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "tuple field %llu has no name", (unsigned long long)idx);
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  /* Set the field value in-place using write_field directly */
  comptime_value_write_field(tuple_val, f->field.offset, f->field.type, value_val);
  return _eval_temp(eval, comptime_value_create_nil(eval->allocator, ctx->builtin_void));
}
