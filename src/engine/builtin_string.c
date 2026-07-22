/**
 * @file builtin_string.c
 * @brief String-related builtin functions: toString.
 */

#include "engine/builtin_string.h"
#include "engine/comptime_eval_internal.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "engine/diagnostic.h"
#include "cubec/expression_call.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"
#include <stdio.h>
#include <string.h>

/* ===== toString core: value → string representation ===== */

/**
 * @brief Recursively convert a comptime value to its string representation.
 *        This is a safe operation — it never fails and always produces a string.
 *
 * - bool/int/float/char/string/pointer: standard formatting
 * - enum: variant name
 * - array: recursive element toString, joined by ','
 * - slice: same as array
 * - struct/union/cunion with toString instance method: delegate
 * - struct/union/cunion without toString: "{struct TypeName 0xADDR}"
 * - other: fallback representation
 */
static comptime_value_t _to_string_value(struct comptime_eval *eval,
                                          struct checker *ctx,
                                          comptime_value_t arg,
                                          node_t node_for_loc) {
  char buf[128];

  switch (arg->kind) {
  case COMPTIME_VALUE_BOOL:
    return _eval_temp(eval, comptime_value_create_string(eval->allocator,
        arg->bool_val ? "true" : "false", ctx->builtin_str));

  case COMPTIME_VALUE_INT:
    if (arg->int_val.is_signed)
      snprintf(buf, sizeof(buf), "%lld", (long long)arg->int_val.s);
    else
      snprintf(buf, sizeof(buf), "%llu", (unsigned long long)arg->int_val.u);
    return _eval_temp(eval, comptime_value_create_string(eval->allocator, buf,
                                                         ctx->builtin_str));

  case COMPTIME_VALUE_FLOAT:
    snprintf(buf, sizeof(buf), "%.6f", arg->float_val.value);
    return _eval_temp(eval, comptime_value_create_string(eval->allocator, buf,
                                                         ctx->builtin_str));

  case COMPTIME_VALUE_CHAR:
    buf[0] = arg->char_val;
    buf[1] = '\0';
    return _eval_temp(eval, comptime_value_create_string(eval->allocator, buf,
                                                         ctx->builtin_str));

  case COMPTIME_VALUE_STRING:
    /* identity */
    return _eval_temp(eval, comptime_value_clone(eval->allocator, arg));

  case COMPTIME_VALUE_POINTER: {
    snprintf(buf, sizeof(buf), "0x%llx", (unsigned long long)arg->pointer.addr);
    return _eval_temp(eval, comptime_value_create_string(eval->allocator, buf,
                                                         ctx->builtin_str));
  }

  case COMPTIME_VALUE_COMPOSITE: {
    if (arg->type) {
      semantic_type_t unq = semantic_type_strip_qualifier(arg->type);

      /* Enum: return variant name */
      if (unq->impl->kind == TYPE_ENUM) {
        uint64_t tag = comptime_value_get_union_tag(arg);
        vec_t items = unq->impl->enum_type.items;
        if (items) {
          size_t ic = vec_get_size(items);
          for (size_t i = 0; i < ic; i++) {
            struct symbol *it = (struct symbol *)vec_get(items, i);
            if (it && it->kind == SYMBOL_ENUM_ITEM &&
                (uint64_t)it->enum_item.value == tag) {
              return _eval_temp(eval, comptime_value_create_string(
                  eval->allocator, it->name, ctx->builtin_str));
            }
          }
        }
        /* Tag not found among enum items — fallback to numeric */
        snprintf(buf, sizeof(buf), "%llu", (unsigned long long)tag);
        return _eval_temp(eval, comptime_value_create_string(
            eval->allocator, buf, ctx->builtin_str));
      }

      /* Array: recursive element toString, joined by ', ' */
      if (arg->composite.element_type) {
        size_t elem_size = arg->composite.element_type->impl
            ? arg->composite.element_type->impl->size : 0;
        size_t count = (elem_size > 0 && arg->composite.data_size >= elem_size)
            ? arg->composite.data_size / elem_size : 0;
        string_t builder = (string_t)allocator_create(
            eval->allocator, &g_string_type, NULL);
        for (size_t i = 0; i < count; i++) {
          if (i > 0) string_concat(builder, ", ");
          comptime_value_t elem = _eval_temp(eval, comptime_value_get_index(
              arg, i, eval->allocator));
          if (elem && elem->kind != COMPTIME_VALUE_ERROR) {
            comptime_value_t elem_str = _to_string_value(eval, ctx, elem,
                                                          node_for_loc);
            if (elem_str && elem_str->kind == COMPTIME_VALUE_STRING) {
              string_concat(builder, comptime_value_get_string(elem_str));
            } else {
              string_concat(builder, "?");
            }
          } else {
            string_concat(builder, "?");
          }
        }
        comptime_value_t result = _eval_temp(eval, comptime_value_create_string(
            eval->allocator, string_get(builder), ctx->builtin_str));
        allocator_free(eval->allocator, &builder);
        return result;
      }

      /* struct/union: call toString() instance method if available */
      if (arg->type->instance_methods) {
        size_t mc = vec_get_size(arg->type->instance_methods);
        for (size_t i = 0; i < mc; i++) {
          struct symbol *m = (struct symbol *)vec_get(arg->type->instance_methods, i);
          if (m && m->name && strcmp(m->name, "toString") == 0 &&
              m->kind == SYMBOL_FUNCTION) {
            comptime_value_t result = _eval_method_call(eval, ctx, m,
                node_for_loc, arg, NULL, 0, node_for_loc);
            if (result && result->kind != COMPTIME_VALUE_ERROR)
              return result;
            break;
          }
        }
      }

      /* struct/union/cunion without toString: {struct TypeName 0xADDR} */
      {
        const char *kind_name = "struct";
        if (unq->impl->kind == TYPE_UNION || unq->impl->kind == TYPE_CUNION)
          kind_name = "union";
        const char *type_name = arg->type->name ? arg->type->name : "<anonymous>";
        /* Use data pointer as identity address */
        snprintf(buf, sizeof(buf), "{%s %s 0x%llx}", kind_name, type_name,
                 (unsigned long long)(uintptr_t)arg->composite.data);
        return _eval_temp(eval, comptime_value_create_string(
            eval->allocator, buf, ctx->builtin_str));
      }
    }
    /* Composite without type info — fallback */
    return _eval_temp(eval, comptime_value_create_string(
        eval->allocator, "<composite>", ctx->builtin_str));
  }

  case COMPTIME_VALUE_FUNCTION:
    return _eval_temp(eval, comptime_value_create_string(
        eval->allocator, "<function>", ctx->builtin_str));

  case COMPTIME_VALUE_TYPE:
    return _eval_temp(eval, comptime_value_create_string(
        eval->allocator,
        arg->type_val && arg->type_val->name ? arg->type_val->name : "<type>",
        ctx->builtin_str));

  case COMPTIME_VALUE_PACK:
    return _eval_temp(eval, comptime_value_create_string(
        eval->allocator, "<pack>", ctx->builtin_str));

  default:
    return _eval_temp(eval, comptime_value_create_string(
        eval->allocator, "<unknown>", ctx->builtin_str));
  }
}

/* ===== toString eval callback ===== */

struct comptime_value *builtin_toString_eval(struct comptime_eval *eval,
                                            struct checker *ctx, node_t node,
                                            struct builtin_entry *be) {
  (void)be;
  cubec_expression_call_t call = (cubec_expression_call_t)node;
  size_t acount = call->arguments ? vec_get_size(call->arguments) : 0;
  if (acount < 1) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "toString() requires at least 1 argument");
    ctx->error_count++;
    return _eval_error_val(eval);
  }
  comptime_value_t arg =
      _comptime_eval_expr(eval, ctx, (node_t)vec_get(call->arguments, 0));
  if (_val_is_error(arg))
    return _eval_error_val(eval);

  return _to_string_value(eval, ctx, arg, node);
}

/* ===== init ===== */

void builtin_table_init_string(builtin_table_t table, struct checker *ctx) {
  /* builtin func toString[T](obj: T): str */
  semantic_type_t t_param = semantic_type_create_generic_param(
      ctx->allocator, "T", 0, NULL, false);
  type_hash_ensure(t_param);
  vec_push(ctx->all_types, t_param);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  vec_push(params, t_param);
  semantic_type_t toStr_type = semantic_type_create_function(
      ctx->allocator, ctx->builtin_str, params, false);
  type_hash_ensure(toStr_type);
  vec_push(ctx->all_types, toStr_type);
  builtin_table_register(table, "toString", toStr_type, builtin_toString_eval);
}
