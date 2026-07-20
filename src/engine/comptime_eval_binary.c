#include "engine/comptime_eval_binary.h"
#include "engine/comptime_eval_internal.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "core/string.h"
#include "core/allocator.h"
#include "cubec/expression_binary.h"
#include <string.h>

static bool _is_comptime_numeric(comptime_value_t v) {
  return v && (v->kind == COMPTIME_VALUE_INT || v->kind == COMPTIME_VALUE_FLOAT);
}

comptime_value_t _comptime_eval_binary(comptime_eval_t eval, checker_t ctx,
                                        node_t node) {
  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  const char *op = string_get(bin->opt);

  /* Prefix unary */
  if (!bin->left) {
    comptime_value_t rv = _comptime_eval_expr(eval, ctx, bin->right);
    if (!rv || rv->kind == COMPTIME_VALUE_ERROR) return _eval_error_val(eval);

    comptime_value_t result = NULL;
    if (strcmp(op, "!") == 0)
      result = comptime_value_create_bool(eval->allocator,
                                          !comptime_value_is_truthy(rv),
                                          rv->type);
    else if (strcmp(op, "-") == 0) {
      if (rv->kind == COMPTIME_VALUE_INT)
        result = comptime_value_create_int(eval->allocator, -rv->int_val.s,
                                           (uint64_t)(-rv->int_val.s),
                                           rv->int_val.width,
                                           rv->int_val.is_signed, rv->type);
      else if (rv->kind == COMPTIME_VALUE_FLOAT)
        result = comptime_value_create_float(eval->allocator, -rv->float_val.value,
                                             rv->float_val.width, rv->type);
    } else if (strcmp(op, "~") == 0) {
      if (rv->kind == COMPTIME_VALUE_INT)
        result = comptime_value_create_int(eval->allocator,
                                           (int64_t)(~rv->int_val.u),
                                           ~rv->int_val.u,
                                           rv->int_val.width,
                                           rv->int_val.is_signed, rv->type);
    }
    return result ? _eval_temp(eval, result) : _eval_error_val(eval);
  }

  /* Binary */
  comptime_value_t lv = _comptime_eval_expr(eval, ctx, bin->left);
  comptime_value_t rv = _comptime_eval_expr(eval, ctx, bin->right);
  if (!lv || lv->kind == COMPTIME_VALUE_ERROR || !rv ||
      rv->kind == COMPTIME_VALUE_ERROR) {
    return _eval_error_val(eval);
  }

  /* __value__ fallback: unwrap non-numeric operands for arithmetic/comparison/bitwise */
  if (!_is_comptime_numeric(lv) && lv->type && lv->type->instance_methods) {
    size_t mc = vec_get_size(lv->type->instance_methods);
    for (size_t i = 0; i < mc; i++) {
      struct symbol *s = (struct symbol *)vec_get(lv->type->instance_methods, i);
      if (s && s->name && strcmp(s->name, "__value__") == 0 &&
          s->kind == SYMBOL_FUNCTION) {
        lv = _eval_method_call(eval, ctx, s, bin->left, lv, NULL, 0, node);
        if (!lv || lv->kind == COMPTIME_VALUE_ERROR) return _eval_error_val(eval);
        break;
      }
    }
  }
  if (!_is_comptime_numeric(rv) && rv->type && rv->type->instance_methods) {
    size_t mc = vec_get_size(rv->type->instance_methods);
    for (size_t i = 0; i < mc; i++) {
      struct symbol *s = (struct symbol *)vec_get(rv->type->instance_methods, i);
      if (s && s->name && strcmp(s->name, "__value__") == 0 &&
          s->kind == SYMBOL_FUNCTION) {
        rv = _eval_method_call(eval, ctx, s, bin->right, rv, NULL, 0, node);
        if (!rv || rv->kind == COMPTIME_VALUE_ERROR) return _eval_error_val(eval);
        break;
      }
    }
  }

  comptime_value_t result = NULL;

  /* Pointer arithmetic is forbidden in Cubec */
  if ((lv->kind == COMPTIME_VALUE_POINTER || rv->kind == COMPTIME_VALUE_POINTER) &&
      (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
       strcmp(op, "*") == 0 || strcmp(op, "/") == 0 || strcmp(op, "%") == 0)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "pointer arithmetic is forbidden in Cubec");
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  /* Arithmetic */
  if (strcmp(op, "+") == 0) {
    if (lv->kind == COMPTIME_VALUE_INT && rv->kind == COMPTIME_VALUE_INT) {
      int64_t s = lv->int_val.s + rv->int_val.s;
      result = comptime_value_create_int(eval->allocator, s, (uint64_t)s,
                                         lv->int_val.width,
                                         lv->int_val.is_signed, lv->type);
    } else if (lv->kind == COMPTIME_VALUE_FLOAT || rv->kind == COMPTIME_VALUE_FLOAT)
      result = comptime_value_create_float(eval->allocator,
                                           comptime_value_as_f64(lv) +
                                               comptime_value_as_f64(rv),
                                           64, ctx->builtin_f64);
  } else if (strcmp(op, "-") == 0) {
    if (lv->kind == COMPTIME_VALUE_INT && rv->kind == COMPTIME_VALUE_INT) {
      int64_t s = lv->int_val.s - rv->int_val.s;
      result = comptime_value_create_int(eval->allocator, s, (uint64_t)s,
                                         lv->int_val.width,
                                         lv->int_val.is_signed, lv->type);
    } else if (lv->kind == COMPTIME_VALUE_FLOAT || rv->kind == COMPTIME_VALUE_FLOAT)
      result = comptime_value_create_float(eval->allocator,
                                           comptime_value_as_f64(lv) -
                                               comptime_value_as_f64(rv),
                                           64, ctx->builtin_f64);
  } else if (strcmp(op, "*") == 0) {
    if (lv->kind == COMPTIME_VALUE_INT && rv->kind == COMPTIME_VALUE_INT) {
      int64_t s = lv->int_val.s * rv->int_val.s;
      result = comptime_value_create_int(eval->allocator, s, (uint64_t)s,
                                         lv->int_val.width,
                                         lv->int_val.is_signed, lv->type);
    } else if (lv->kind == COMPTIME_VALUE_FLOAT || rv->kind == COMPTIME_VALUE_FLOAT)
      result = comptime_value_create_float(eval->allocator,
                                           comptime_value_as_f64(lv) *
                                               comptime_value_as_f64(rv),
                                           64, ctx->builtin_f64);
  } else if (strcmp(op, "/") == 0) {
    if (rv->kind == COMPTIME_VALUE_INT &&
        ((rv->int_val.is_signed && rv->int_val.s == 0) ||
         (!rv->int_val.is_signed && rv->int_val.u == 0))) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                           "division by zero");
      ctx->error_count++;
    } else if (lv->kind == COMPTIME_VALUE_INT && rv->kind == COMPTIME_VALUE_INT) {
      int64_t s = lv->int_val.is_signed ? lv->int_val.s / rv->int_val.s
                                         : (int64_t)(lv->int_val.u / rv->int_val.u);
      result = comptime_value_create_int(eval->allocator, s, (uint64_t)s,
                                         lv->int_val.width,
                                         lv->int_val.is_signed, lv->type);
    } else if (lv->kind == COMPTIME_VALUE_FLOAT || rv->kind == COMPTIME_VALUE_FLOAT)
      result = comptime_value_create_float(eval->allocator,
                                           comptime_value_as_f64(lv) /
                                               comptime_value_as_f64(rv),
                                           64, ctx->builtin_f64);
  } else if (strcmp(op, "%") == 0) {
    if (rv->kind == COMPTIME_VALUE_INT &&
        ((rv->int_val.is_signed && rv->int_val.s == 0) ||
         (!rv->int_val.is_signed && rv->int_val.u == 0))) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                           "modulo by zero");
      ctx->error_count++;
    } else if (lv->kind == COMPTIME_VALUE_INT && rv->kind == COMPTIME_VALUE_INT) {
      int64_t s = lv->int_val.is_signed ? lv->int_val.s % rv->int_val.s
                                         : (int64_t)(lv->int_val.u % rv->int_val.u);
      result = comptime_value_create_int(eval->allocator, s, (uint64_t)s,
                                         lv->int_val.width,
                                         lv->int_val.is_signed, lv->type);
    }
  }

  /* Comparison */
  else if (strcmp(op, "==") == 0)
    result = comptime_value_create_bool(eval->allocator,
                                        comptime_value_equals(lv, rv), lv->type);
  else if (strcmp(op, "!=") == 0)
    result = comptime_value_create_bool(eval->allocator,
                                        !comptime_value_equals(lv, rv), lv->type);

  /* Relational */
  else if (strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
           strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
    bool bresult = false;
    if (lv->kind == COMPTIME_VALUE_INT && rv->kind == COMPTIME_VALUE_INT) {
      if (lv->int_val.is_signed && rv->int_val.is_signed) {
        if (strcmp(op, "<") == 0)  bresult = lv->int_val.s < rv->int_val.s;
        if (strcmp(op, "<=") == 0) bresult = lv->int_val.s <= rv->int_val.s;
        if (strcmp(op, ">") == 0)  bresult = lv->int_val.s > rv->int_val.s;
        if (strcmp(op, ">=") == 0) bresult = lv->int_val.s >= rv->int_val.s;
      } else {
        if (strcmp(op, "<") == 0)  bresult = lv->int_val.u < rv->int_val.u;
        if (strcmp(op, "<=") == 0) bresult = lv->int_val.u <= rv->int_val.u;
        if (strcmp(op, ">") == 0)  bresult = lv->int_val.u > rv->int_val.u;
        if (strcmp(op, ">=") == 0) bresult = lv->int_val.u >= rv->int_val.u;
      }
    } else {
      double lf = comptime_value_as_f64(lv), rf = comptime_value_as_f64(rv);
      if (strcmp(op, "<") == 0)  bresult = lf < rf;
      if (strcmp(op, "<=") == 0) bresult = lf <= rf;
      if (strcmp(op, ">") == 0)  bresult = lf > rf;
      if (strcmp(op, ">=") == 0) bresult = lf >= rf;
    }
    result = comptime_value_create_bool(eval->allocator, bresult, ctx->builtin_bool);
  }

  /* Logical */
  else if (strcmp(op, "&&") == 0)
    result = comptime_value_create_bool(eval->allocator,
                                        comptime_value_is_truthy(lv) &&
                                            comptime_value_is_truthy(rv),
                                        ctx->builtin_bool);
  else if (strcmp(op, "||") == 0)
    result = comptime_value_create_bool(eval->allocator,
                                        comptime_value_is_truthy(lv) ||
                                            comptime_value_is_truthy(rv),
                                        ctx->builtin_bool);

  /* Bitwise */
  else if (strcmp(op, "&") == 0 && lv->kind == COMPTIME_VALUE_INT &&
           rv->kind == COMPTIME_VALUE_INT)
    result = comptime_value_create_int(eval->allocator,
                                       (int64_t)(lv->int_val.u & rv->int_val.u),
                                       lv->int_val.u & rv->int_val.u,
                                       lv->int_val.width,
                                       lv->int_val.is_signed, lv->type);
  else if (strcmp(op, "|") == 0 && lv->kind == COMPTIME_VALUE_INT &&
           rv->kind == COMPTIME_VALUE_INT)
    result = comptime_value_create_int(eval->allocator,
                                       (int64_t)(lv->int_val.u | rv->int_val.u),
                                       lv->int_val.u | rv->int_val.u,
                                       lv->int_val.width,
                                       lv->int_val.is_signed, lv->type);
  else if (strcmp(op, "^") == 0 && lv->kind == COMPTIME_VALUE_INT &&
           rv->kind == COMPTIME_VALUE_INT)
    result = comptime_value_create_int(eval->allocator,
                                       (int64_t)(lv->int_val.u ^ rv->int_val.u),
                                       lv->int_val.u ^ rv->int_val.u,
                                       lv->int_val.width,
                                       lv->int_val.is_signed, lv->type);
  else if (strcmp(op, "<<") == 0 && lv->kind == COMPTIME_VALUE_INT &&
           rv->kind == COMPTIME_VALUE_INT)
    result = comptime_value_create_int(eval->allocator,
                                       (int64_t)(lv->int_val.u << rv->int_val.u),
                                       lv->int_val.u << rv->int_val.u,
                                       lv->int_val.width,
                                       lv->int_val.is_signed, lv->type);
  else if (strcmp(op, ">>") == 0 && lv->kind == COMPTIME_VALUE_INT &&
           rv->kind == COMPTIME_VALUE_INT) {
    uint64_t shifted = lv->int_val.is_signed
                           ? (uint64_t)(lv->int_val.s >> rv->int_val.u)
                           : lv->int_val.u >> rv->int_val.u;
    result = comptime_value_create_int(eval->allocator, (int64_t)shifted, shifted,
                                       lv->int_val.width,
                                       lv->int_val.is_signed, lv->type);
  }

  return result ? _eval_temp(eval, result) : _eval_error_val(eval);
}
