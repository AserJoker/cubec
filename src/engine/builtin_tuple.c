/**
 * @file builtin_tuple.c
 * @brief Tuple-related builtin functions: getTupleItem, setTupleItem.
 */

#include "engine/builtin_tuple.h"
#include "engine/comptime_eval_internal.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "engine/diagnostic.h"
#include "cubec/expression_call.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/literal_numeric.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"

/* ===== getTupleItem eval callback ===== */

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
  if (_val_is_error(tuple_val))
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
  if (!tuple_type || tuple_type->impl->kind != TYPE_TUPLE ||
      !tuple_type->impl->tuple.fields) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "getTupleItem() requires a tuple argument");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  vec_t fields = tuple_type->impl->tuple.fields;
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

/* ===== setTupleItem eval callback ===== */

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
  if (_val_is_error(tuple_val) ||
      _val_is_error(value_val))
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
  if (!tuple_type || tuple_type->impl->kind != TYPE_TUPLE ||
      !tuple_type->impl->tuple.fields) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->location,
                         "set() requires a tuple argument");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  vec_t fields = tuple_type->impl->tuple.fields;
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
  comptime_value_write_field(tuple_val, f->field.offset, f->field.type, value_val, eval->allocator);
  return _eval_temp(eval, comptime_value_create_nil(eval->allocator, ctx->builtin_void));
}

/* ===== init ===== */

void builtin_table_init_tuple(builtin_table_t table, struct checker *ctx) {
  /* builtin func getTupleItem[N: u64, ...Args](tuple: <...Args>): Args[N] */
  {
    semantic_type_t n_param = semantic_type_create_generic_param(
        ctx->allocator, "N", 0, ctx->builtin_u64, true);
    type_hash_ensure(n_param);
    vec_push(ctx->all_types, n_param);

    semantic_type_t args_param = semantic_type_create_generic_pack(
        ctx->allocator, "Args", 1);
    type_hash_ensure(args_param);
    vec_push(ctx->all_types, args_param);

    /* Params: tuple: <...Args> — a TYPE_TUPLE with element types from the pack */
    vec_init_t evi = {.auto_dispose = false};
    vec_t elem_types = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &evi);
    vec_push(elem_types, args_param);
    semantic_type_t tuple_param_type = semantic_type_create_tuple(
        ctx->allocator, elem_types);
    type_hash_ensure(tuple_param_type);
    vec_push(ctx->all_types, tuple_param_type);

    vec_init_t vi2 = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi2);
    vec_push(params, tuple_param_type);

    semantic_type_t ret_type = semantic_type_create_pack_index(
        ctx->allocator, "Args", 1, 0);
    type_hash_ensure(ret_type);
    vec_push(ctx->all_types, ret_type);

    semantic_type_t get_type = semantic_type_create_function(
        ctx->allocator, ret_type, params, false);
    type_hash_ensure(get_type);
    vec_push(ctx->all_types, get_type);
    builtin_table_register(table, "getTupleItem", get_type, builtin_get_eval);
  }

  /* builtin func setTupleItem[N: u64, ...Args](tuple: <...Args>, value: Args[N]): void */
  {
    semantic_type_t n_param_s = semantic_type_create_generic_param(
        ctx->allocator, "N", 0, ctx->builtin_u64, true);
    type_hash_ensure(n_param_s);
    vec_push(ctx->all_types, n_param_s);

    semantic_type_t args_param_s = semantic_type_create_generic_pack(
        ctx->allocator, "Args", 1);
    type_hash_ensure(args_param_s);
    vec_push(ctx->all_types, args_param_s);

    /* Params: tuple: <...Args> */
    vec_init_t evi_s = {.auto_dispose = false};
    vec_t elem_types_s = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &evi_s);
    vec_push(elem_types_s, args_param_s);
    semantic_type_t tuple_param_type_s = semantic_type_create_tuple(
        ctx->allocator, elem_types_s);
    type_hash_ensure(tuple_param_type_s);
    vec_push(ctx->all_types, tuple_param_type_s);

    semantic_type_t value_type = semantic_type_create_pack_index(
        ctx->allocator, "Args", 1, 0);
    type_hash_ensure(value_type);
    vec_push(ctx->all_types, value_type);

    vec_init_t vi_s2 = {.auto_dispose = false};
    vec_t params_s = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi_s2);
    vec_push(params_s, tuple_param_type_s);
    vec_push(params_s, value_type);

    semantic_type_t set_type = semantic_type_create_function(
        ctx->allocator, ctx->builtin_void, params_s, false);
    type_hash_ensure(set_type);
    vec_push(ctx->all_types, set_type);
    builtin_table_register(table, "setTupleItem", set_type, builtin_set_eval);
  }
}
