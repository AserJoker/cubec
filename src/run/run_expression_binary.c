#include "run/run.h"
#include "engine/vm.h"
#include "engine/exception_type.h"
#include "engine/bool_type.h"
#include "engine/value.h"
#include "cubec/expression_binary.h"
#include "core/string.h"
#include <string.h>

/* ---- Unary ---- */

static value_t _run_unary(context_t ctx, const char *op, node_t right,
                          bool shadow) {
  vm_t vm = ctx->vm;
  value_t rv = run_expression(ctx, right, shadow);
  if (value_is_error(rv)) return rv;

  if (strcmp(op, "!") == 0) return value_lnot(vm, rv);
  if (strcmp(op, "+") == 0) return value_pos(vm, rv);
  if (strcmp(op, "-") == 0) return value_neg(vm, rv);
  if (strcmp(op, "~") == 0) return value_bnot(vm, rv);

  return create_exception_value(vm, "run: unknown unary operator '%s'", op);
}

/* ---- Binary ---- */

static value_t _run_binary(context_t ctx, const char *op, node_t left,
                           node_t right, bool shadow) {
  vm_t vm = ctx->vm;

  /* short-circuit: && and || */
  if (strcmp(op, "&&") == 0) {
    value_t lv = run_expression(ctx, left, shadow);
    if (value_is_error(lv)) return lv;
    if (value_is_shadow(lv))
      return vm_create_value_shadow(vm,
                                    (type_t)value_get_data(vm_get_bool_type(vm)),
                                    NULL, true);
    bool lb = *(bool *)value_get_data(lv);
    if (!lb) return create_bool_value(vm, false);
    value_t rv = run_expression(ctx, right, shadow);
    if (value_is_error(rv)) return rv;
    /* coerce to bool via double lnot */
    return value_lnot(vm, value_lnot(vm, rv));
  }
  if (strcmp(op, "||") == 0) {
    value_t lv = run_expression(ctx, left, shadow);
    if (value_is_error(lv)) return lv;
    if (value_is_shadow(lv))
      return vm_create_value_shadow(vm,
                                    (type_t)value_get_data(vm_get_bool_type(vm)),
                                    NULL, true);
    bool lb = *(bool *)value_get_data(lv);
    if (lb) return create_bool_value(vm, true);
    value_t rv = run_expression(ctx, right, shadow);
    if (value_is_error(rv)) return rv;
    return value_lnot(vm, value_lnot(vm, rv));
  }

  /* eager: evaluate both */
  value_t lv = run_expression(ctx, left, shadow);
  if (value_is_error(lv)) return lv;
  value_t rv = run_expression(ctx, right, shadow);
  if (value_is_error(rv)) return rv;

  /* arithmetic */
  if (strcmp(op, "+") == 0) return value_add(vm, lv, rv);
  if (strcmp(op, "-") == 0) return value_sub(vm, lv, rv);
  if (strcmp(op, "*") == 0) return value_mul(vm, lv, rv);
  if (strcmp(op, "/") == 0) return value_div(vm, lv, rv);
  if (strcmp(op, "%") == 0) return value_mod(vm, lv, rv);

  /* bitwise */
  if (strcmp(op, "&") == 0) return value_band(vm, lv, rv);
  if (strcmp(op, "|") == 0) return value_bor(vm, lv, rv);
  if (strcmp(op, "^") == 0) return value_bxor(vm, lv, rv);
  if (strcmp(op, "<<") == 0) return value_shl(vm, lv, rv);
  if (strcmp(op, ">>") == 0) return value_shr(vm, lv, rv);

  /* comparison */
  if (strcmp(op, "==") == 0) return value_equal(vm, lv, rv);
  if (strcmp(op, "!=") == 0) {
    value_t eq = value_equal(vm, lv, rv);
    if (value_is_error(eq)) return eq;
    return value_lnot(vm, eq);
  }
  if (strcmp(op, "<") == 0) return value_lt(vm, lv, rv);
  if (strcmp(op, ">") == 0) return value_gt(vm, lv, rv);
  if (strcmp(op, "<=") == 0) {
    value_t gt = value_gt(vm, lv, rv);
    if (value_is_error(gt)) return gt;
    return value_lnot(vm, gt);
  }
  if (strcmp(op, ">=") == 0) {
    value_t lt = value_lt(vm, lv, rv);
    if (value_is_error(lt)) return lt;
    return value_lnot(vm, lt);
  }

  return create_exception_value(vm, "run: unknown binary operator '%s'", op);
}

value_t run_expression_binary(context_t ctx, node_t node, bool shadow) {
  cubec_expression_binary_t bin = (cubec_expression_binary_t)node;
  const char *op = string_get(bin->opt);

  /* unary: left is NULL */
  if (!bin->left)
    return _run_unary(ctx, op, bin->right, shadow);

  return _run_binary(ctx, op, bin->left, bin->right, shadow);
}
