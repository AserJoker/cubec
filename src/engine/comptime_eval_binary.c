#include "engine/comptime_eval_binary.h"
#include "engine/resolver.h"
#include "core/string.h"
#include "core/allocator.h"
#include "cubec/expression_binary.h"
#include <string.h>

comptime_value_t _comptime_eval_binary(comptime_eval_t eval, checker_t ctx,
                                        node_t node) {
  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  const char *op = string_get(bin->opt);

  /* Prefix unary */
  if (!bin->left) {
    comptime_value_t rv = _comptime_eval_expr(eval, ctx, bin->right);
    if (!rv || rv->kind == COMPTIME_VALUE_ERROR) {
      allocator_free(eval->allocator, &rv);
      return _eval_error_val(eval);
    }

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
    allocator_free(eval->allocator, &rv);
    return result ? result : _eval_error_val(eval);
  }

  /* Binary */
  comptime_value_t lv = _comptime_eval_expr(eval, ctx, bin->left);
  comptime_value_t rv = _comptime_eval_expr(eval, ctx, bin->right);
  if (!lv || lv->kind == COMPTIME_VALUE_ERROR || !rv ||
      rv->kind == COMPTIME_VALUE_ERROR) {
    allocator_free(eval->allocator, &lv);
    allocator_free(eval->allocator, &rv);
    return _eval_error_val(eval);
  }

  comptime_value_t result = NULL;

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

  allocator_free(eval->allocator, &lv);
  allocator_free(eval->allocator, &rv);
  return result ? result : _eval_error_val(eval);
}
