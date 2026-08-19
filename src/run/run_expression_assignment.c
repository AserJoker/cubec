#include "run/run.h"
#include "engine/vm.h"
#include "engine/exception_type.h"
#include "engine/void_type.h"
#include "engine/value.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_member.h"
#include "cubec/expression_deref.h"
#include "cubec/expression_subscript.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/literal_identifier.h"
#include "core/string.h"
#include "core/vec.h"
#include <string.h>

/* ---- lvalue read helpers (for compound assignment) ---- */

static value_t _lvalue_read_identifier(vm_t vm, node_t node,
                                       bool shadow) {
  return run_literal_identifier(vm, node, shadow);
}

static value_t _lvalue_read_member(vm_t vm, node_t node, bool shadow) {
  cubec_expression_member_t mem = (cubec_expression_member_t)node;
  value_t host = run_expression(vm, mem->host, shadow);
  if (value_is_abnormal(host)) return host;
  return value_get_field(vm, host, string_get(mem->field->value));
}

static value_t _lvalue_read_deref(vm_t vm, node_t node, bool shadow) {
  cubec_expression_deref_t deref = (cubec_expression_deref_t)node;
  value_t host = run_expression(vm, deref->host, shadow);
  if (value_is_abnormal(host)) return host;
  return value_deref_get(vm, host);
}

static value_t _lvalue_read_subscript(vm_t vm, node_t node,
                                      bool shadow) {
  cubec_expression_subscript_t sub = (cubec_expression_subscript_t)node;
  value_t host = run_expression(vm, sub->host, shadow);
  if (value_is_abnormal(host)) return host;
  /* TODO: validate host is not a generic name (not yet implemented) */
  size_t argc = vec_get_size(sub->arguments);
  if (argc != 1)
    return create_exception_value(vm,
                                  "run: subscript requires exactly one argument, got %zu",
                                  argc);
  node_t index_node = (node_t)vec_get(sub->arguments, 0);
  value_t index = run_expression(vm, index_node, shadow);
  if (value_is_abnormal(index)) return index;
  return value_get_item(vm, host, index);
}

static value_t _lvalue_read_namespace_access(vm_t vm, node_t node,
                                             bool shadow) {
  cubec_expression_namespace_access_t ns =
      (cubec_expression_namespace_access_t)node;
  value_t host = run_expression(vm, ns->host, shadow);
  if (value_is_abnormal(host)) return host;
  return value_get_prop(vm, host, string_get(ns->field->value));
}

static value_t _lvalue_read(vm_t vm, node_t lvalue, bool shadow) {
  switch (lvalue->kind) {
  case CUBEC_NODE_LITERAL_IDENTIFIER:
    return _lvalue_read_identifier(vm, lvalue, shadow);
  case CUBEC_NODE_EXPRESSION_MEMBER:
    return _lvalue_read_member(vm, lvalue, shadow);
  case CUBEC_NODE_EXPRESSION_DEREF:
    return _lvalue_read_deref(vm, lvalue, shadow);
  case CUBEC_NODE_EXPRESSION_SUBSCRIPT:
    return _lvalue_read_subscript(vm, lvalue, shadow);
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS:
    return _lvalue_read_namespace_access(vm, lvalue, shadow);
  default:
    return create_exception_value(vm,
                                  "run: invalid lvalue node kind %d",
                                  lvalue->kind);
  }
}

/* ---- lvalue write helpers — return void on success ---- */

static value_t _lvalue_write_identifier(vm_t vm, node_t node,
                                        value_t rv) {
  value_t lv = run_literal_identifier(vm, node, false);
  if (value_is_abnormal(lv)) return lv;
  value_t result = value_assignment(vm, lv, rv);
  if (value_is_abnormal(result)) return result;
  return create_void_value(vm);
}

static value_t _lvalue_write_member(vm_t vm, node_t node, value_t rv) {
  cubec_expression_member_t mem = (cubec_expression_member_t)node;
  value_t host = run_expression(vm, mem->host, false);
  if (value_is_abnormal(host)) return host;
  value_t result = value_set_field(vm, host, string_get(mem->field->value), rv);
  if (value_is_abnormal(result)) return result;
  return create_void_value(vm);
}

static value_t _lvalue_write_deref(vm_t vm, node_t node, value_t rv) {
  cubec_expression_deref_t deref = (cubec_expression_deref_t)node;
  value_t host = run_expression(vm, deref->host, false);
  if (value_is_abnormal(host)) return host;
  value_t result = value_deref_set(vm, host, rv);
  if (value_is_abnormal(result)) return result;
  return create_void_value(vm);
}

static value_t _lvalue_write_subscript(vm_t vm, node_t node,
                                       value_t rv) {
  cubec_expression_subscript_t sub = (cubec_expression_subscript_t)node;
  value_t host = run_expression(vm, sub->host, false);
  if (value_is_abnormal(host)) return host;
  /* TODO: validate host is not a generic name (not yet implemented) */
  size_t argc = vec_get_size(sub->arguments);
  if (argc != 1)
    return create_exception_value(vm,
                                  "run: subscript requires exactly one argument, got %zu",
                                  argc);
  node_t index_node = (node_t)vec_get(sub->arguments, 0);
  value_t index = run_expression(vm, index_node, false);
  if (value_is_abnormal(index)) return index;
  value_t result = value_set_item(vm, host, index, rv);
  if (value_is_abnormal(result)) return result;
  return create_void_value(vm);
}

static value_t _lvalue_write_namespace_access(vm_t vm, node_t node,
                                              value_t rv) {
  cubec_expression_namespace_access_t ns =
      (cubec_expression_namespace_access_t)node;
  value_t host = run_expression(vm, ns->host, false);
  if (value_is_abnormal(host)) return host;
  value_t result = value_set_prop(vm, host, string_get(ns->field->value), rv);
  if (value_is_abnormal(result)) return result;
  return create_void_value(vm);
}

static value_t _lvalue_write(vm_t vm, node_t lvalue, value_t rv) {
  switch (lvalue->kind) {
  case CUBEC_NODE_LITERAL_IDENTIFIER:
    return _lvalue_write_identifier(vm, lvalue, rv);
  case CUBEC_NODE_EXPRESSION_MEMBER:
    return _lvalue_write_member(vm, lvalue, rv);
  case CUBEC_NODE_EXPRESSION_DEREF:
    return _lvalue_write_deref(vm, lvalue, rv);
  case CUBEC_NODE_EXPRESSION_SUBSCRIPT:
    return _lvalue_write_subscript(vm, lvalue, rv);
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS:
    return _lvalue_write_namespace_access(vm, lvalue, rv);
  default:
    return create_exception_value(vm,
                                  "run: invalid lvalue node kind %d",
                                  lvalue->kind);
  }
}

/* ---- compound assignment: compute op(lv_current, rv) ---- */

static value_t _compound_apply(vm_t vm, const char *op, value_t lv,
                               value_t rv) {
  if (strcmp(op, "+=") == 0) return value_add(vm, lv, rv);
  if (strcmp(op, "-=") == 0) return value_sub(vm, lv, rv);
  if (strcmp(op, "*=") == 0) return value_mul(vm, lv, rv);
  if (strcmp(op, "/=") == 0) return value_div(vm, lv, rv);
  if (strcmp(op, "%=") == 0) return value_mod(vm, lv, rv);
  if (strcmp(op, "&=") == 0) return value_band(vm, lv, rv);
  if (strcmp(op, "|=") == 0) return value_bor(vm, lv, rv);
  if (strcmp(op, "^=") == 0) return value_bxor(vm, lv, rv);
  if (strcmp(op, "<<=") == 0) return value_shl(vm, lv, rv);
  if (strcmp(op, ">>=") == 0) return value_shr(vm, lv, rv);
  if (strcmp(op, "&&=") == 0) {
    value_t b = value_lnot(vm, value_lnot(vm, rv));
    if (value_is_abnormal(b)) return b;
    return value_lnot(vm, value_lnot(vm, b));
  }
  if (strcmp(op, "||=") == 0) {
    value_t b = value_lnot(vm, value_lnot(vm, rv));
    if (value_is_abnormal(b)) return b;
    return value_lnot(vm, value_lnot(vm, b));
  }
  return create_exception_value(vm,
                                "run: unknown compound operator '%s'", op);
}

/* ---- check if identifier is discard wildcard '_' ---- */

static bool _is_discard_wildcard(node_t node) {
  if (node->kind != CUBEC_NODE_LITERAL_IDENTIFIER)
    return false;
  cubec_literal_identifier_t id = (cubec_literal_identifier_t)node;
  return strcmp(string_get(id->value), "_") == 0;
}

/* ---- main entry ---- */

value_t run_expression_assignment(vm_t vm, node_t node, bool shadow) {
  cubec_expression_assignment_t asgn = (cubec_expression_assignment_t)node;
  const char *op = string_get(asgn->opt);

  /* discard wildcard: _ = expr — evaluate right side, discard result */
  if (_is_discard_wildcard(asgn->left)) {
    if (shadow) return create_void_value(vm);
    value_t rv = run_expression(vm, asgn->right, false);
    if (value_is_abnormal(rv)) return rv;
    return create_void_value(vm);
  }

  /* validate lvalue kind */
  switch (asgn->left->kind) {
  case CUBEC_NODE_LITERAL_IDENTIFIER:
  case CUBEC_NODE_EXPRESSION_MEMBER:
  case CUBEC_NODE_EXPRESSION_DEREF:
  case CUBEC_NODE_EXPRESSION_SUBSCRIPT:
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS:
    break;
  default:
    return create_exception_value(vm,
                                  "run: invalid lvalue node kind %d",
                                  asgn->left->kind);
  }

  /* shadow: compute type only, skip write */
  if (shadow) {
    value_t lv = _lvalue_read(vm, asgn->left, true);
    if (value_is_abnormal(lv)) return lv;
    if (strcmp(op, "=") == 0) {
      value_t rv = run_expression(vm, asgn->right, true);
      return rv;
    }
    value_t rv = run_expression(vm, asgn->right, true);
    if (value_is_abnormal(rv)) return rv;
    return _compound_apply(vm, op, lv, rv);
  }

  /* simple assignment: = */
  if (strcmp(op, "=") == 0) {
    value_t rv = run_expression(vm, asgn->right, false);
    if (value_is_abnormal(rv)) return rv;
    value_t result = _lvalue_write(vm, asgn->left, rv);
    return result;
  }

  /* compound assignment: read -> compute -> write */
  value_t lv = _lvalue_read(vm, asgn->left, false);
  if (value_is_abnormal(lv)) return lv;
  value_t rv = run_expression(vm, asgn->right, false);
  if (value_is_abnormal(rv)) return rv;
  value_t computed = _compound_apply(vm, op, lv, rv);
  if (value_is_abnormal(computed)) return computed;
  return _lvalue_write(vm, asgn->left, computed);
}
