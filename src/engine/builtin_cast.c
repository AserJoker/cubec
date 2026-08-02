/**
 * @file builtin_cast.c
 * @brief Cast builtin function: func cast[T,K](expr:K):T
 *
 * Conversion rules implemented in builtin_cast_eval:
 *   Numeric: float→int (truncation), int narrowing, float narrowing,
 *            bool↔int, enum↔int, char↔int
 *   Pointer: opaque→pointer, pointer→int, []T→*T, *Small→*Big (downcast)
 *   Container: array→tuple (layout-compatible)
 */

#include "engine/builtin_cast.h"
#include "cubec/expression_call.h"
#include "cubec/expression_generic_instantiation.h"
#include "engine/comptime_eval_internal.h"
#include "engine/resolver.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
#include <string.h>


/* ===== cast eval: numeric conversions ===== */

/**
 * @brief Create a truncated int value for the target integer type.
 *        Handles width truncation and sign extension correctly.
 */
static comptime_value_t _create_truncated_int(struct comptime_eval *eval,
                                              uint64_t raw_uval,
                                              semantic_type_t to) {
  semantic_type_t to_unq = semantic_type_strip_qualifier(to);
  bool is_signed =
      (to_unq->impl->kind >= TYPE_I8 && to_unq->impl->kind <= TYPE_I64);
  uint8_t width = to_unq->impl->size * 8;
  uint64_t uval = raw_uval;

  /* Truncate to target width */
  if (width < 64) {
    uint64_t mask = (1ULL << width) - 1;
    uval = uval & mask;
  }

  /* Sign-extend for signed types */
  int64_t sval;
  if (is_signed && width < 64) {
    uint64_t sign_bit = 1ULL << (width - 1);
    if (uval & sign_bit)
      sval = (int64_t)(uval | ~((1ULL << width) - 1));
    else
      sval = (int64_t)uval;
  } else {
    sval = (int64_t)uval;
  }

  return comptime_value_create_int(eval->allocator, sval, uval, width,
                                   is_signed, to);
}

static comptime_value_t _cast_numeric(struct comptime_eval *eval,
                                      struct context *ctx, semantic_type_t from,
                                      semantic_type_t to,
                                      comptime_value_t src_val) {
  semantic_type_t from_unq = semantic_type_strip_qualifier(from);
  semantic_type_t to_unq = semantic_type_strip_qualifier(to);
  enum type_kind fk = from_unq->impl->kind;
  enum type_kind tk = to_unq->impl->kind;

  /* float → int (truncation) */
  if (fk >= TYPE_F16 && fk <= TYPE_F64 && tk >= TYPE_I8 && tk <= TYPE_U64) {
    double d = comptime_value_as_f64(src_val);
    uint64_t uval = (uint64_t)(int64_t)d;
    return _eval_temp(eval, _create_truncated_int(eval, uval, to));
  }

  /* int → float (explicit cast) */
  if (fk >= TYPE_I8 && fk <= TYPE_U64 && tk >= TYPE_F16 && tk <= TYPE_F64) {
    double d = (double)comptime_value_as_f64(src_val);
    uint8_t width =
        to_unq->impl->size == 8 ? 64 : (to_unq->impl->size == 4 ? 32 : 16);
    return _eval_temp(
        eval, comptime_value_create_float(eval->allocator, d, width, to));
  }

  /* int → int (narrowing or same-width) */
  if (fk >= TYPE_I8 && fk <= TYPE_U64 && tk >= TYPE_I8 && tk <= TYPE_U64) {
    uint64_t uval = comptime_value_as_u64(src_val);
    return _eval_temp(eval, _create_truncated_int(eval, uval, to));
  }

  /* float → float (narrowing) */
  if (fk >= TYPE_F16 && fk <= TYPE_F64 && tk >= TYPE_F16 && tk <= TYPE_F64) {
    double d = comptime_value_as_f64(src_val);
    uint8_t width =
        to_unq->impl->size == 8 ? 64 : (to_unq->impl->size == 4 ? 32 : 16);
    return _eval_temp(
        eval, comptime_value_create_float(eval->allocator, d, width, to));
  }

  /* bool → int */
  if (fk == TYPE_BOOL && tk >= TYPE_I8 && tk <= TYPE_U64) {
    uint64_t uval = comptime_value_is_truthy(src_val) ? 1 : 0;
    bool is_signed = (tk >= TYPE_I8 && tk <= TYPE_I64);
    uint8_t width = to_unq->impl->size * 8;
    return _eval_temp(eval,
                      comptime_value_create_int(eval->allocator, (int64_t)uval,
                                                uval, width, is_signed, to));
  }

  /* int → bool */
  if (fk >= TYPE_I8 && fk <= TYPE_U64 && tk == TYPE_BOOL) {
    bool b = comptime_value_as_u64(src_val) != 0;
    return _eval_temp(eval, comptime_value_create_bool(eval->allocator, b, to));
  }

  /* enum → int */
  if (fk == TYPE_ENUM && tk >= TYPE_I8 && tk <= TYPE_U64) {
    uint64_t uval = comptime_value_as_u64(src_val);
    return _eval_temp(eval, _create_truncated_int(eval, uval, to));
  }

  /* int → enum */
  if (fk >= TYPE_I8 && fk <= TYPE_U64 && tk == TYPE_ENUM) {
    uint64_t uval = comptime_value_as_u64(src_val);
    semantic_type_t backing = to_unq->impl->enum_type.backing_type;
    if (!backing)
      backing = ctx->builtin_i32;
    bool is_signed =
        (backing->impl->kind >= TYPE_I8 && backing->impl->kind <= TYPE_I64);
    uint8_t width = backing->impl->size * 8;
    return _eval_temp(eval,
                      comptime_value_create_int(eval->allocator, (int64_t)uval,
                                                uval, width, is_signed, to));
  }

  /* char → int */
  if (fk == TYPE_CHAR && tk >= TYPE_I8 && tk <= TYPE_U64) {
    uint64_t uval = (uint64_t)(unsigned char)src_val->char_val;
    return _eval_temp(eval, _create_truncated_int(eval, uval, to));
  }

  /* int → char */
  if (fk >= TYPE_I8 && fk <= TYPE_U64 && tk == TYPE_CHAR) {
    char c = (char)comptime_value_as_u64(src_val);
    return _eval_temp(
        eval, comptime_value_create_literal_char(eval->allocator, c, to));
  }

  return NULL; /* not a numeric conversion */
}

/* ===== cast eval: pointer conversions ===== */

static comptime_value_t _cast_pointer(struct comptime_eval *eval,
                                      struct context *ctx, semantic_type_t from,
                                      semantic_type_t to,
                                      comptime_value_t src_val) {
  (void)ctx;
  semantic_type_t from_unq = semantic_type_strip_qualifier(from);
  semantic_type_t to_unq = semantic_type_strip_qualifier(to);

  /* opaque → pointer */
  if (from_unq->impl->kind == TYPE_OPAQUE &&
      to_unq->impl->kind == TYPE_POINTER) {
    uint64_t addr =
        src_val->kind == COMPTIME_VALUE_POINTER ? src_val->pointer.addr : 0;
    return _eval_temp(eval,
                      comptime_value_create_pointer(eval->allocator, addr, to));
  }

  /* pointer → int */
  if (from_unq->impl->kind == TYPE_POINTER && to_unq->impl->kind >= TYPE_I8 &&
      to_unq->impl->kind <= TYPE_U64) {
    uint64_t addr =
        src_val->kind == COMPTIME_VALUE_POINTER ? src_val->pointer.addr : 0;
    return _eval_temp(eval, _create_truncated_int(eval, addr, to));
  }

  /* slice → pointer: []T → *T (extract data pointer with start offset applied)
   */
  if (from_unq->impl->kind == TYPE_SLICE &&
      to_unq->impl->kind == TYPE_POINTER) {
    if (src_val->kind == COMPTIME_VALUE_COMPOSITE && src_val->composite.data) {
      const size_t ptr_size = 8; /* matches type_layout_compute default */
      /* Read data pointer at offset 0 */
      uint64_t base_addr = 0;
      memcpy(&base_addr, src_val->composite.data, ptr_size);
      /* Read start at offset ptr_size */
      uint64_t start = 0;
      memcpy(&start, src_val->composite.data + ptr_size, ptr_size);
      /* Apply start offset: result = base_addr + start * sizeof(T) */
      semantic_type_t elem_type = from_unq->impl->slice.element;
      type_layout_compute(elem_type, 8);
      uint64_t result_addr = base_addr + start * elem_type->impl->size;
      return _eval_temp(eval, comptime_value_create_pointer(eval->allocator,
                                                            result_addr, to));
    }
    /* Non-composite slice value: return zero pointer */
    return _eval_temp(eval,
                      comptime_value_create_pointer(eval->allocator, 0, to));
  }

  /* *Small → *Big (struct pointer downcast) */
  if (from_unq->impl->kind == TYPE_POINTER &&
      to_unq->impl->kind == TYPE_POINTER) {
    semantic_type_t from_pt =
        semantic_type_strip_qualifier(from_unq->impl->pointer.pointee);
    semantic_type_t to_pt =
        semantic_type_strip_qualifier(to_unq->impl->pointer.pointee);
    vec_t from_fields = NULL;
    vec_t to_fields = NULL;
    if (from_pt->impl->kind == TYPE_STRUCT ||
        from_pt->impl->kind == TYPE_UNION || from_pt->impl->kind == TYPE_CUNION)
      from_fields = from_pt->impl->struct_type.fields;
    else if (from_pt->impl->kind == TYPE_GENERIC_INSTANCE)
      from_fields = from_pt->impl->generic_instance.fields;
    if (to_pt->impl->kind == TYPE_STRUCT || to_pt->impl->kind == TYPE_UNION ||
        to_pt->impl->kind == TYPE_CUNION)
      to_fields = to_pt->impl->struct_type.fields;
    else if (to_pt->impl->kind == TYPE_GENERIC_INSTANCE)
      to_fields = to_pt->impl->generic_instance.fields;

    if (from_fields && to_fields) {
      size_t fc = vec_get_size(from_fields);
      size_t tc = vec_get_size(to_fields);
      if (fc <= tc) {
        bool prefix_ok = true;
        for (size_t i = 0; i < fc && prefix_ok; i++) {
          struct symbol *ff = (struct symbol *)vec_get(from_fields, i);
          struct symbol *tf = (struct symbol *)vec_get(to_fields, i);
          if (!ff || !tf)
            prefix_ok = false;
          else if (!ff->name || !tf->name || strcmp(ff->name, tf->name) != 0)
            prefix_ok = false;
          else if (!semantic_type_equals(ff->field.type, tf->field.type))
            prefix_ok = false;
        }
        if (prefix_ok) {
          uint64_t addr = src_val->kind == COMPTIME_VALUE_POINTER
                              ? src_val->pointer.addr
                              : 0;
          return _eval_temp(
              eval, comptime_value_create_pointer(eval->allocator, addr, to));
        }
      }
    }
  }

  return NULL; /* not a pointer conversion */
}

/* ===== cast eval: container conversions ===== */

static comptime_value_t _cast_container(struct comptime_eval *eval,
                                        struct context *ctx,
                                        semantic_type_t from,
                                        semantic_type_t to,
                                        comptime_value_t src_val, node_t node) {
  semantic_type_t from_unq = semantic_type_strip_qualifier(from);
  semantic_type_t to_unq = semantic_type_strip_qualifier(to);

  /* array → tuple (layout-compatible) */
  if (from_unq->impl->kind == TYPE_ARRAY && to_unq->impl->kind == TYPE_TUPLE) {
    if (src_val->kind != COMPTIME_VALUE_COMPOSITE) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                           "cast: expected array value");
      ctx->error_count++;
      return _eval_error_val(eval);
    }
    semantic_type_t elem = from_unq->impl->array.element;
    vec_t to_elems = to_unq->impl->tuple.element_types;
    size_t arr_len = from_unq->impl->array.length;
    size_t tup_len = vec_get_size(to_elems);
    if (arr_len != tup_len) {
      diagnostic_list_push(
          ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
          "cast: array length %llu does not match tuple size %llu",
          (unsigned long long)arr_len, (unsigned long long)tup_len);
      ctx->error_count++;
      return _eval_error_val(eval);
    }
    for (size_t i = 0; i < tup_len; i++) {
      semantic_type_t te = (semantic_type_t)vec_get(to_elems, i);
      if (semantic_type_get_size(te) != semantic_type_get_size(elem) ||
          semantic_type_get_alignment(te) !=
              semantic_type_get_alignment(elem)) {
        diagnostic_list_push(
            ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
            "cast: array element layout incompatible with tuple element %llu",
            (unsigned long long)i);
        ctx->error_count++;
        return _eval_error_val(eval);
      }
    }
    comptime_value_t result = comptime_value_create_composite(
        eval->allocator, to, elem, src_val->composite.data_size);
    if (result && src_val->composite.data && src_val->composite.data_size > 0)
      memcpy(result->composite.data, src_val->composite.data,
             src_val->composite.data_size);
    return _eval_temp(eval, result);
  }

  return NULL; /* not a container conversion */
}

/* ===== main eval callback ===== */

struct comptime_value *builtin_cast_eval(struct comptime_eval *eval,
                                         struct context *ctx, node_t node,
                                         struct builtin_entry *be) {
  (void)be;
  cubec_expression_call_t call = (cubec_expression_call_t)node;
  size_t acount = call->arguments ? vec_get_size(call->arguments) : 0;
  if (acount < 1) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "cast() requires at least 1 argument");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  /* Evaluate the argument */
  comptime_value_t src_val =
      _comptime_eval_expr(eval, ctx, (node_t)vec_get(call->arguments, 0));
  if (_val_is_error(src_val))
    return _eval_error_val(eval);

  /* Get target type T from the generic instantiation's first type argument.
     For cast[i32](3.14), the callee is a generic_instantiation node whose
     arguments[0] is the type expression for T. We resolve it directly. */
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
          /* Fallback: resolve type args using the generic param list */
          vec_t gp = NULL;
          const char *name = _checker_ident_str(gi->callee);
          struct symbol *sym =
              name ? scope_lookup(ctx->current_scope, name) : NULL;
          if (sym && sym->kind == SYMBOL_FUNCTION &&
              sym->function.generic_params) {
            gp = sym->function.generic_params;
          }
          vec_t resolved_args =
              _resolve_generic_type_args(ctx, gi->arguments, gp);
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
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "cast: could not resolve target type");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  semantic_type_t source_type = src_val->type;
  if (!source_type || source_type->impl->kind == TYPE_ERROR)
    return _eval_error_val(eval);

  /* If already implicitly convertible, return value with target type */
  if (semantic_type_can_implicit_convert(source_type, target_type)) {
    /* __value__ fallback: if struct-like source has __value__ and standard
       conversions don't directly apply, unwrap via __value__ first.
       Magic methods only exist on struct/union/cunion/generic_instance types.
     */
    if (src_val->kind != COMPTIME_VALUE_INT &&
        src_val->kind != COMPTIME_VALUE_FLOAT &&
        src_val->kind != COMPTIME_VALUE_BOOL &&
        src_val->kind != COMPTIME_VALUE_CHAR &&
        src_val->kind != COMPTIME_VALUE_POINTER) {
      semantic_type_t src_unq = semantic_type_strip_qualifier(source_type);
      bool src_struct = src_unq->impl->kind == TYPE_STRUCT ||
                        src_unq->impl->kind == TYPE_UNION ||
                        src_unq->impl->kind == TYPE_CUNION ||
                        src_unq->impl->kind == TYPE_GENERIC_INSTANCE;
      if (src_struct && source_type->instance_methods) {
        size_t mc = vec_get_size(source_type->instance_methods);
        for (size_t i = 0; i < mc; i++) {
          struct symbol *s =
              (struct symbol *)vec_get(source_type->instance_methods, i);
          if (s && s->name && strcmp(s->name, "__value__") == 0 &&
              s->kind == SYMBOL_FUNCTION) {
            /* Call __value__ to get the unboxed value */
            node_t host_node = (node_t)vec_get(call->arguments, 0);
            comptime_value_t unboxed = _eval_method_call(
                eval, ctx, s, host_node, src_val, NULL, 0, node);
            if (unboxed && unboxed->kind != COMPTIME_VALUE_ERROR) {
              /* Use unboxed value to continue conversion */
              semantic_type_t unboxed_type = unboxed->type;
              if (unboxed_type && semantic_type_can_implicit_convert(
                                      unboxed_type, target_type)) {
                switch (unboxed->kind) {
                case COMPTIME_VALUE_INT:
                  return _eval_temp(
                      eval, comptime_value_create_int(
                                eval->allocator, unboxed->int_val.s,
                                unboxed->int_val.u, unboxed->int_val.width,
                                unboxed->int_val.is_signed, target_type));
                case COMPTIME_VALUE_FLOAT:
                  return _eval_temp(
                      eval, comptime_value_create_float(
                                eval->allocator, unboxed->float_val.value,
                                unboxed->float_val.width, target_type));
                case COMPTIME_VALUE_BOOL:
                  return _eval_temp(eval, comptime_value_create_bool(
                                              eval->allocator,
                                              unboxed->bool_val, target_type));
                default:
                  return _eval_temp(
                      eval, comptime_value_clone(eval->allocator, unboxed));
                }
              }
            }
            break;
          }
        }
      }
    }
    switch (src_val->kind) {
    case COMPTIME_VALUE_INT:
      return _eval_temp(eval, comptime_value_create_int(
                                  eval->allocator, src_val->int_val.s,
                                  src_val->int_val.u, src_val->int_val.width,
                                  src_val->int_val.is_signed, target_type));
    case COMPTIME_VALUE_FLOAT:
      return _eval_temp(eval, comptime_value_create_float(
                                  eval->allocator, src_val->float_val.value,
                                  src_val->float_val.width, target_type));
    case COMPTIME_VALUE_BOOL:
      return _eval_temp(eval, comptime_value_create_bool(eval->allocator,
                                                         src_val->bool_val,
                                                         target_type));
    case COMPTIME_VALUE_CHAR:
      return _eval_temp(eval,
                        comptime_value_create_literal_char(
                            eval->allocator, src_val->char_val, target_type));
    case COMPTIME_VALUE_POINTER:
      return _eval_temp(
          eval, comptime_value_create_pointer(
                    eval->allocator, src_val->pointer.addr, target_type));
    default:
      return _eval_temp(eval, comptime_value_clone(eval->allocator, src_val));
    }
  }

  /* Try explicit conversions */
  comptime_value_t result;

  result = _cast_numeric(eval, ctx, source_type, target_type, src_val);
  if (result)
    return result;

  result = _cast_pointer(eval, ctx, source_type, target_type, src_val);
  if (result)
    return result;

  result = _cast_container(eval, ctx, source_type, target_type, src_val, node);
  if (result)
    return result;

  /* No valid conversion found */
  diagnostic_list_push(
      ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
      "cannot cast '%s' to '%s'",
      semantic_type_get_name(source_type) ? semantic_type_get_name(source_type)
                                          : "?",
      semantic_type_get_name(target_type) ? semantic_type_get_name(target_type)
                                          : "?");
  ctx->error_count++;
  return _eval_error_val(eval);
}

/* ===== init ===== */

void builtin_table_init_cast(builtin_table_t table, struct context *ctx) {
  /* builtin func cast[T,K](expr:K):T */
  semantic_type_t t_param =
      semantic_type_create_generic_param(ctx->allocator, "T", NULL, false);
  type_hash_ensure(t_param);
  vec_push(ctx->all_types, t_param);

  semantic_type_t k_param =
      semantic_type_create_generic_param(ctx->allocator, "K", NULL, false);
  type_hash_ensure(k_param);
  vec_push(ctx->all_types, k_param);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  vec_push(params, k_param);

  semantic_type_t cast_type =
      semantic_type_create_function(ctx->allocator, t_param, params, false);
  type_hash_ensure(cast_type);
  vec_push(ctx->all_types, cast_type);
  builtin_table_register(table, "cast", cast_type, builtin_cast_eval);
}
