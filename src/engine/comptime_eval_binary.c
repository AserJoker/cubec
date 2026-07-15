#include "engine/comptime_eval_binary.h"
#include "engine/resolver.h"
#include "core/string.h"
#include "cubec/expression_binary.h"
#include <string.h>

comptime_value_t _comptime_eval_binary(comptime_eval_t eval, checker_t ctx,
                                        node_t node) {
  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  const char *op = string_get(bin->opt);

  /* Prefix unary */
  if (!bin->left) {
    comptime_value_t rv = _comptime_eval_expr(eval, ctx, bin->right);
    if (!rv || rv->kind == COMPTIME_VALUE_ERROR) return _eval_error_val(eval);

    if (strcmp(op, "!") == 0)
      return comptime_value_create_bool(eval->allocator,
                                         !comptime_value_is_truthy(rv),
                                         rv->type);
    if (strcmp(op, "-") == 0) {
      if (rv->kind == COMPTIME_VALUE_INT)
        return comptime_value_create_int(eval->allocator, -rv->int_val.s,
                                          (uint64_t)(-rv->int_val.s),
                                          rv->int_val.width,
                                          rv->int_val.is_signed, rv->type);
      if (rv->kind == COMPTIME_VALUE_FLOAT)
        return comptime_value_create_float(eval->allocator, -rv->float_val.value,
                                            rv->float_val.width, rv->type);
      return _eval_error_val(eval);
    }
    if (strcmp(op, "~") == 0) {
      if (rv->kind == COMPTIME_VALUE_INT)
        return comptime_value_create_int(eval->allocator,
                                          (int64_t)(~rv->int_val.u),
                                          ~rv->int_val.u,
                                          rv->int_val.width,
                                          rv->int_val.is_signed, rv->type);
      return _eval_error_val(eval);
    }
    return _eval_error_val(eval);
  }

  /* Binary */
  comptime_value_t lv = _comptime_eval_expr(eval, ctx, bin->left);
  comptime_value_t rv = _comptime_eval_expr(eval, ctx, bin->right);
  if (!lv || lv->kind == COMPTIME_VALUE_ERROR || !rv ||
      rv->kind == COMPTIME_VALUE_ERROR)
    return _eval_error_val(eval);

  /* Arithmetic */
  if (strcmp(op, "+") == 0) {
    if (lv->kind == COMPTIME_VALUE_INT && rv->kind == COMPTIME_VALUE_INT) {
      int64_t s = lv->int_val.s + rv->int_val.s;
      return comptime_value_create_int(eval->allocator, s, (uint64_t)s,
                                        lv->int_val.width,
                                        lv->int_val.is_signed, lv->type);
    }
    if (lv->kind == COMPTIME_VALUE_FLOAT || rv->kind == COMPTIME_VALUE_FLOAT)
      return comptime_value_create_float(eval->allocator,
                                          comptime_value_as_f64(lv) +
                                              comptime_value_as_f64(rv),
                                          64, ctx->builtin_f64);
    return _eval_error_val(eval);
  }
  if (strcmp(op, "-") == 0) {
    if (lv->kind == COMPTIME_VALUE_INT && rv->kind == COMPTIME_VALUE_INT) {
      int64_t s = lv->int_val.s - rv->int_val.s;
      return comptime_value_create_int(eval->allocator, s, (uint64_t)s,
                                        lv->int_val.width,
                                        lv->int_val.is_signed, lv->type);
    }
    if (lv->kind == COMPTIME_VALUE_FLOAT || rv->kind == COMPTIME_VALUE_FLOAT)
      return comptime_value_create_float(eval->allocator,
                                          comptime_value_as_f64(lv) -
                                              comptime_value_as_f64(rv),
                                          64, ctx->builtin_f64);
    return _eval_error_val(eval);
  }
  if (strcmp(op, "*") == 0) {
    if (lv->kind == COMPTIME_VALUE_INT && rv->kind == COMPTIME_VALUE_INT) {
      int64_t s = lv->int_val.s * rv->int_val.s;
      return comptime_value_create_int(eval->allocator, s, (uint64_t)s,
                                        lv->int_val.width,
                                        lv->int_val.is_signed, lv->type);
    }
    if (lv->kind == COMPTIME_VALUE_FLOAT || rv->kind == COMPTIME_VALUE_FLOAT)
      return comptime_value_create_float(eval->allocator,
                                          comptime_value_as_f64(lv) *
                                              comptime_value_as_f64(rv),
                                          64, ctx->builtin_f64);
    return _eval_error_val(eval);
  }
  if (strcmp(op, "/") == 0) {
    if (rv->kind == COMPTIME_VALUE_INT &&
        ((rv->int_val.is_signed && rv->int_val.s == 0) ||
         (!rv->int_val.is_signed && rv->int_val.u == 0))) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                           "division by zero");
      ctx->error_count++;
      return _eval_error_val(eval);
    }
    if (lv->kind == COMPTIME_VALUE_INT && rv->kind == COMPTIME_VALUE_INT) {
      int64_t s = lv->int_val.is_signed ? lv->int_val.s / rv->int_val.s
                                         : (int64_t)(lv->int_val.u / rv->int_val.u);
      return comptime_value_create_int(eval->allocator, s, (uint64_t)s,
                                        lv->int_val.width,
                                        lv->int_val.is_signed, lv->type);
    }
    if (lv->kind == COMPTIME_VALUE_FLOAT || rv->kind == COMPTIME_VALUE_FLOAT)
      return comptime_value_create_float(eval->allocator,
                                          comptime_value_as_f64(lv) /
                                              comptime_value_as_f64(rv),
                                          64, ctx->builtin_f64);
    return _eval_error_val(eval);
  }
  if (strcmp(op, "%") == 0) {
    if (rv->kind == COMPTIME_VALUE_INT &&
        ((rv->int_val.is_signed && rv->int_val.s == 0) ||
         (!rv->int_val.is_signed && rv->int_val.u == 0))) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                           "modulo by zero");
      ctx->error_count++;
      return _eval_error_val(eval);
    }
    if (lv->kind == COMPTIME_VALUE_INT && rv->kind == COMPTIME_VALUE_INT) {
      int64_t s = lv->int_val.is_signed ? lv->int_val.s % rv->int_val.s
                                         : (int64_t)(lv->int_val.u % rv->int_val.u);
      return comptime_value_create_int(eval->allocator, s, (uint64_t)s,
                                        lv->int_val.width,
                                        lv->int_val.is_signed, lv->type);
    }
    return _eval_error_val(eval);
  }

  /* Comparison */
  if (strcmp(op, "==") == 0)
    return comptime_value_create_bool(eval->allocator,
                                       comptime_value_equals(lv, rv), lv->type);
  if (strcmp(op, "!=") == 0)
    return comptime_value_create_bool(eval->allocator,
                                       !comptime_value_equals(lv, rv), lv->type);

  /* Relational */
  if (strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
      strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
    bool result = false;
    if (lv->kind == COMPTIME_VALUE_INT && rv->kind == COMPTIME_VALUE_INT) {
      if (lv->int_val.is_signed && rv->int_val.is_signed) {
        if (strcmp(op, "<") == 0)  result = lv->int_val.s < rv->int_val.s;
        if (strcmp(op, "<=") == 0) result = lv->int_val.s <= rv->int_val.s;
        if (strcmp(op, ">") == 0)  result = lv->int_val.s > rv->int_val.s;
        if (strcmp(op, ">=") == 0) result = lv->int_val.s >= rv->int_val.s;
      } else {
        if (strcmp(op, "<") == 0)  result = lv->int_val.u < rv->int_val.u;
        if (strcmp(op, "<=") == 0) result = lv->int_val.u <= rv->int_val.u;
        if (strcmp(op, ">") == 0)  result = lv->int_val.u > rv->int_val.u;
        if (strcmp(op, ">=") == 0) result = lv->int_val.u >= rv->int_val.u;
      }
    } else {
      double lf = comptime_value_as_f64(lv), rf = comptime_value_as_f64(rv);
      if (strcmp(op, "<") == 0)  result = lf < rf;
      if (strcmp(op, "<=") == 0) result = lf <= rf;
      if (strcmp(op, ">") == 0)  result = lf > rf;
      if (strcmp(op, ">=") == 0) result = lf >= rf;
    }
    return comptime_value_create_bool(eval->allocator, result, ctx->builtin_bool);
  }

  /* Logical */
  if (strcmp(op, "&&") == 0)
    return comptime_value_create_bool(eval->allocator,
                                       comptime_value_is_truthy(lv) &&
                                           comptime_value_is_truthy(rv),
                                       ctx->builtin_bool);
  if (strcmp(op, "||") == 0)
    return comptime_value_create_bool(eval->allocator,
                                       comptime_value_is_truthy(lv) ||
                                           comptime_value_is_truthy(rv),
                                       ctx->builtin_bool);

  /* Bitwise */
  if (strcmp(op, "&") == 0 && lv->kind == COMPTIME_VALUE_INT &&
      rv->kind == COMPTIME_VALUE_INT)
    return comptime_value_create_int(eval->allocator,
                                      (int64_t)(lv->int_val.u & rv->int_val.u),
                                      lv->int_val.u & rv->int_val.u,
                                      lv->int_val.width,
                                      lv->int_val.is_signed, lv->type);
  if (strcmp(op, "|") == 0 && lv->kind == COMPTIME_VALUE_INT &&
      rv->kind == COMPTIME_VALUE_INT)
    return comptime_value_create_int(eval->allocator,
                                      (int64_t)(lv->int_val.u | rv->int_val.u),
                                      lv->int_val.u | rv->int_val.u,
                                      lv->int_val.width,
                                      lv->int_val.is_signed, lv->type);
  if (strcmp(op, "^") == 0 && lv->kind == COMPTIME_VALUE_INT &&
      rv->kind == COMPTIME_VALUE_INT)
    return comptime_value_create_int(eval->allocator,
                                      (int64_t)(lv->int_val.u ^ rv->int_val.u),
                                      lv->int_val.u ^ rv->int_val.u,
                                      lv->int_val.width,
                                      lv->int_val.is_signed, lv->type);
  if (strcmp(op, "<<") == 0 && lv->kind == COMPTIME_VALUE_INT &&
      rv->kind == COMPTIME_VALUE_INT)
    return comptime_value_create_int(eval->allocator,
                                      (int64_t)(lv->int_val.u << rv->int_val.u),
                                      lv->int_val.u << rv->int_val.u,
                                      lv->int_val.width,
                                      lv->int_val.is_signed, lv->type);
  if (strcmp(op, ">>") == 0 && lv->kind == COMPTIME_VALUE_INT &&
      rv->kind == COMPTIME_VALUE_INT) {
    uint64_t shifted = lv->int_val.is_signed
                           ? (uint64_t)(lv->int_val.s >> rv->int_val.u)
                           : lv->int_val.u >> rv->int_val.u;
    return comptime_value_create_int(eval->allocator, (int64_t)shifted, shifted,
                                      lv->int_val.width,
                                      lv->int_val.is_signed, lv->type);
  }

  return _eval_error_val(eval);
}
