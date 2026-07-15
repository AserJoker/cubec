#include "engine/checker.h"
#include "engine/checker_check_expr.h"
#include "engine/checker_check_expr_helpers.h"
#include "engine/checker_type_util.h"
#include "engine/checker_check_stmt.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"
#include "cubec/node.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include "cubec/literal_char.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_postfix_unary.h"
#include "cubec/expression_call.h"
#include "cubec/expression_member.h"
#include "cubec/expression_ternary.h"
#include "cubec/expression_group.h"
#include "cubec/expression_sizeof.h"
#include "cubec/expression_alignof.h"
#include "cubec/expression_typeof.h"
#include "cubec/expression_slice.h"
#include "cubec/expression_function.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/expression_comma.h"
#include "cubec/expression_spread.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_type_qualifier.h"
#include <string.h>

/* ===== expression checker sub-functions ===== */

static semantic_type_t _check_expr_literal_identifier(checker_t ctx, node_t expr) {
  const char *name = _checker_ident_str(expr);
  if (!name) return ctx->error_type;
  struct symbol *sym = scope_lookup(ctx->current_scope, name);
  if (!sym) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "undeclared identifier '%s'", name);
    ctx->error_count++;
    return ctx->error_type;
  }
  switch (sym->kind) {
  case SYMBOL_VARIABLE: return sym->variable.type;
  case SYMBOL_FUNCTION: return sym->function.type;
  case SYMBOL_ENUM_ITEM: return sym->enum_item.owning_type;
  case SYMBOL_GENERIC_PARAM:
    if (sym->generic_param.value_type)
      return sym->generic_param.value_type;
    if (sym->generic_param.constraint)
      return sym->generic_param.constraint;
    return ctx->error_type;
  default: return ctx->error_type;
  }
}

static semantic_type_t _check_expr_prefix_unary(checker_t ctx, node_t expr) {
  cubec_expression_binary_t bin = (cubec_expression_binary_t)expr;
  const char *op = string_get(bin->opt);
  semantic_type_t rt = _check_expression(ctx, bin->right);
  if (rt->impl->kind == TYPE_ERROR) return ctx->error_type;

  if (strcmp(op, "!") == 0) {
    if (!_is_bool_type(rt)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           expr->location,
                           "operator '!' requires bool operand");
      ctx->error_count++;
    }
    return ctx->builtin_bool;
  }
  if (strcmp(op, "-") == 0) {
    if (!_is_numeric_type(rt)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           expr->location,
                           "operator '-' requires numeric operand");
      ctx->error_count++;
    }
    return rt;
  }
  if (strcmp(op, "~") == 0) {
    if (!_is_integer_type(rt)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           expr->location,
                           "operator '~' requires integer operand");
      ctx->error_count++;
    }
    return rt;
  }
  return rt;
}

static semantic_type_t _check_expr_binary(checker_t ctx, node_t expr) {
  cubec_expression_binary_t bin = (cubec_expression_binary_t)expr;
  const char *op = string_get(bin->opt);

  if (!bin->left) return _check_expr_prefix_unary(ctx, expr);

  semantic_type_t lt = _check_expression(ctx, bin->left);
  semantic_type_t rt = _check_expression(ctx, bin->right);
  if (lt->impl->kind == TYPE_ERROR || rt->impl->kind == TYPE_ERROR)
    return ctx->error_type;

  static const char *arith_ops[] = {"+", "-", "*", "/", "%"};
  static const char *cmp_ops[] = {"==", "!=", "<", ">", "<=", ">="};
  static const char *logic_ops[] = {"&&", "||"};
  static const char *bit_ops[] = {"&", "|", "^", "<<", ">>"};

  if (_is_op_one_of(op, arith_ops, 5))
    return _check_binary_arithmetic(ctx, expr, op, lt, rt);
  if (_is_op_one_of(op, cmp_ops, 6))
    return ctx->builtin_bool;
  if (_is_op_one_of(op, logic_ops, 2)) {
    if (!_is_bool_type(lt) || !_is_bool_type(rt)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                           "logical operator '%s' requires bool operands", op);
      ctx->error_count++;
    }
    return ctx->builtin_bool;
  }
  if (_is_op_one_of(op, bit_ops, 5))
    return _check_binary_bitwise(ctx, expr, op, lt, rt);

  return ctx->error_type;
}

static semantic_type_t _check_expr_assignment(checker_t ctx, node_t expr) {
  cubec_expression_assignment_t asgn = (cubec_expression_assignment_t)expr;
  const char *op = string_get(asgn->opt);

  if (!_is_lvalue(asgn->left)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "left side of assignment is not a valid lvalue");
    ctx->error_count++;
  }

  semantic_type_t lt = _check_expression(ctx, asgn->left);
  semantic_type_t rt = _check_expression(ctx, asgn->right);
  if (lt->impl->kind == TYPE_ERROR || rt->impl->kind == TYPE_ERROR)
    return ctx->error_type;

  if (asgn->left->kind == CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION) {
    semantic_type_t result = _check_assign_generic_lhs(ctx, expr, lt, rt);
    if (result) return result;
  }

  if (strcmp(op, "=") == 0) {
    if (!semantic_type_can_implicit_convert(rt, lt)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                           "cannot assign '%s' to '%s'",
                           rt->name ? rt->name : "<anonymous>",
                           lt->name ? lt->name : "<anonymous>");
      ctx->error_count++;
    }
    return lt;
  }

  if (!semantic_type_can_implicit_convert(rt, lt)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "type mismatch in compound assignment");
    ctx->error_count++;
  }
  return lt;
}

static semantic_type_t _check_expr_call(checker_t ctx, node_t expr) {
  cubec_expression_call_t call = (cubec_expression_call_t)expr;
  semantic_type_t callee_type = _check_expression(ctx, call->callee);
  if (callee_type->impl->kind == TYPE_ERROR) return ctx->error_type;

  if (callee_type->impl->kind != TYPE_FUNCTION) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "call of non-function type");
    ctx->error_count++;
    return ctx->error_type;
  }

  vec_t params = callee_type->impl->function.params;
  size_t param_count = params ? vec_get_size(params) : 0;
  size_t arg_count = call->arguments ? vec_get_size(call->arguments) : 0;
  bool is_variadic = callee_type->impl->function.is_variadic;

  if (!is_variadic && arg_count != param_count) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "expected %zu arguments, got %zu",
                         param_count, arg_count);
    ctx->error_count++;
  }

  for (size_t i = 0; i < arg_count; i++) {
    node_t arg = (node_t)vec_get(call->arguments, i);
    semantic_type_t at = _check_expression(ctx, arg);
    if (i < param_count && at->impl->kind != TYPE_ERROR) {
      semantic_type_t pt = (semantic_type_t)vec_get(params, i);
      if (!semantic_type_can_implicit_convert(at, pt)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             arg->location,
                             "argument %zu: cannot convert '%s' to '%s'",
                             i + 1,
                             at->name ? at->name : "<anonymous>",
                             pt->name ? pt->name : "<anonymous>");
        ctx->error_count++;
      }
    }
  }

  return callee_type->impl->function.return_type;
}

static semantic_type_t _check_expr_member(checker_t ctx, node_t expr) {
  cubec_expression_member_t mem = (cubec_expression_member_t)expr;
  semantic_type_t host_type = _check_expression(ctx, mem->host);
  if (host_type->impl->kind == TYPE_ERROR) return ctx->error_type;

  const char *fname = _checker_ident_str((node_t)mem->field);
  if (!fname) return ctx->error_type;

  if (host_type->impl->kind == TYPE_STRUCT ||
      host_type->impl->kind == TYPE_UNION ||
      host_type->impl->kind == TYPE_CUNION) {
    vec_t fields = host_type->impl->struct_type.fields;
    size_t fcount = fields ? vec_get_size(fields) : 0;
    for (size_t i = 0; i < fcount; i++) {
      struct symbol *f = (struct symbol *)vec_get(fields, i);
      if (f && f->name && strcmp(f->name, fname) == 0)
        return f->field.type;
    }
    size_t mcount = vec_get_size(host_type->instance_methods);
    for (size_t i = 0; i < mcount; i++) {
      struct symbol *m = (struct symbol *)vec_get(host_type->instance_methods, i);
      if (m && m->name && strcmp(m->name, fname) == 0)
        return m->function.type;
    }
  }

  if (host_type->impl->kind == TYPE_INTERFACE) {
    vec_t methods = host_type->impl->interface_type.methods;
    size_t mcount = methods ? vec_get_size(methods) : 0;
    for (size_t i = 0; i < mcount; i++) {
      struct symbol *m = (struct symbol *)vec_get(methods, i);
      if (m && m->name && strcmp(m->name, fname) == 0)
        return m->function.type;
    }
  }

  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                       "type '%s' has no member '%s'",
                       host_type->name ? host_type->name : "<anonymous>",
                       fname);
  ctx->error_count++;
  return ctx->error_type;
}

static semantic_type_t _check_expr_namespace_access(checker_t ctx, node_t expr) {
  cubec_expression_namespace_access_t ns =
      (cubec_expression_namespace_access_t)expr;
  semantic_type_t host_type = _check_expression(ctx, ns->host);
  if (host_type->impl->kind == TYPE_ERROR) return ctx->error_type;

  const char *fname = _checker_ident_str((node_t)ns->field);
  if (!fname) return ctx->error_type;

  size_t sfcount = vec_get_size(host_type->static_fields);
  for (size_t i = 0; i < sfcount; i++) {
    struct symbol *sf = (struct symbol *)vec_get(host_type->static_fields, i);
    if (sf && sf->name && strcmp(sf->name, fname) == 0)
      return sf->variable.type;
  }
  size_t smcount = vec_get_size(host_type->static_methods);
  for (size_t i = 0; i < smcount; i++) {
    struct symbol *sm = (struct symbol *)vec_get(host_type->static_methods, i);
    if (sm && sm->name && strcmp(sm->name, fname) == 0)
      return sm->function.type;
  }
  size_t atcount = vec_get_size(host_type->associated_types);
  for (size_t i = 0; i < atcount; i++) {
    struct symbol *at = (struct symbol *)vec_get(host_type->associated_types, i);
    if (at && at->name && strcmp(at->name, fname) == 0)
      return at->type.type;
  }

  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                       "type '%s' has no static member '%s'",
                       host_type->name ? host_type->name : "<anonymous>",
                       fname);
  ctx->error_count++;
  return ctx->error_type;
}

static semantic_type_t _check_expr_deref(checker_t ctx, node_t expr) {
  cubec_expression_postfix_unary_t pf =
      (cubec_expression_postfix_unary_t)expr;
  semantic_type_t host_type = _check_expression(ctx, pf->left);
  if (host_type->impl->kind == TYPE_ERROR) return ctx->error_type;

  if (host_type->impl->kind != TYPE_POINTER) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "cannot dereference non-pointer type");
    ctx->error_count++;
    return ctx->error_type;
  }
  return host_type->impl->pointer.pointee;
}

static semantic_type_t _check_expr_addr(checker_t ctx, node_t expr) {
  cubec_expression_postfix_unary_t pf =
      (cubec_expression_postfix_unary_t)expr;
  semantic_type_t host_type = _check_expression(ctx, pf->left);
  if (host_type->impl->kind == TYPE_ERROR) return ctx->error_type;

  if (!_is_lvalue(pf->left)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "cannot take address of non-lvalue");
    ctx->error_count++;
  }
  semantic_type_t pt = semantic_type_create_pointer(ctx->allocator, host_type);
  vec_push(ctx->all_types, pt);
  return pt;
}

static semantic_type_t _check_expr_try(checker_t ctx, node_t expr) {
  cubec_expression_postfix_unary_t pf =
      (cubec_expression_postfix_unary_t)expr;
  semantic_type_t host_type = _check_expression(ctx, pf->left);
  if (host_type->impl->kind == TYPE_ERROR) return ctx->error_type;
  if (host_type->impl->kind == TYPE_POINTER)
    return host_type->impl->pointer.pointee;
  if (host_type->impl->kind == TYPE_INTERFACE) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "cannot unwrap interface type");
    ctx->error_count++;
    return ctx->error_type;
  }
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                       "try operator requires pointer or interface type");
  ctx->error_count++;
  return ctx->error_type;
}

static semantic_type_t _check_expr_ternary(checker_t ctx, node_t expr) {
  cubec_expression_ternary_t tern = (cubec_expression_ternary_t)expr;
  semantic_type_t ct = _check_expression(ctx, tern->condition);
  semantic_type_t tt = _check_expression(ctx, tern->consequent);
  semantic_type_t ft = _check_expression(ctx, tern->alternate);

  if (!_is_bool_type(ct)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "ternary condition must be bool");
    ctx->error_count++;
  }
  return _common_type(ctx, tt, ft);
}

static semantic_type_t _check_expr_group(checker_t ctx, node_t expr) {
  cubec_expression_group_t grp = (cubec_expression_group_t)expr;
  return _check_expression(ctx, grp->inner);
}

static semantic_type_t _check_expr_sizeof(checker_t ctx, node_t expr) {
  cubec_expression_sizeof_t sz = (cubec_expression_sizeof_t)expr;
  semantic_type_t t = resolver_resolve_type(ctx, sz->expression);
  if (t->impl->kind == TYPE_ERROR)
    t = _check_expression(ctx, sz->expression);
  if (t->impl->kind == TYPE_ERROR) return ctx->error_type;
  if (semantic_type_is_incomplete(t)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "sizeof of incomplete type");
    ctx->error_count++;
    return ctx->error_type;
  }
  type_layout_compute(t, 8);
  return ctx->builtin_u64;
}

static semantic_type_t _check_expr_alignof(checker_t ctx, node_t expr) {
  cubec_expression_alignof_t al = (cubec_expression_alignof_t)expr;
  semantic_type_t t = resolver_resolve_type(ctx, al->expression);
  if (t->impl->kind == TYPE_ERROR)
    t = _check_expression(ctx, al->expression);
  if (t->impl->kind == TYPE_ERROR) return ctx->error_type;
  if (semantic_type_is_incomplete(t)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "alignof of incomplete type");
    ctx->error_count++;
    return ctx->error_type;
  }
  type_layout_compute(t, 8);
  return ctx->builtin_u64;
}

static semantic_type_t _check_expr_typeof(checker_t ctx, node_t expr) {
  cubec_expression_typeof_t to = (cubec_expression_typeof_t)expr;
  semantic_type_t inner = _check_expression(ctx, to->expression);
  semantic_type_t t =
      semantic_type_create_named(ctx->allocator, NULL, TYPE_TYPE);
  t->impl->type_of.inner = inner;
  t->is_incomplete = false;
  vec_push(ctx->all_types, t);
  return t;
}

static semantic_type_t _check_expr_slice(checker_t ctx, node_t expr) {
  cubec_expression_slice_t sl = (cubec_expression_slice_t)expr;
  semantic_type_t ht = _check_expression(ctx, sl->host);
  if (ht->impl->kind == TYPE_ERROR) return ctx->error_type;

  if (sl->start) _check_expression(ctx, sl->start);
  if (sl->length) _check_expression(ctx, sl->length);

  if (ht->impl->kind == TYPE_ARRAY) {
    semantic_type_t st = semantic_type_create_slice(ctx->allocator,
                                       ht->impl->array.element);
    vec_push(ctx->all_types, st);
    return st;
  }
  if (ht->impl->kind == TYPE_SLICE)
    return ht;

  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                       "cannot slice type '%s'",
                       ht->name ? ht->name : "<anonymous>");
  ctx->error_count++;
  return ctx->error_type;
}

static semantic_type_t _check_expr_function(checker_t ctx, node_t expr) {
  cubec_expression_function_t fn = (cubec_expression_function_t)expr;
  semantic_type_t ret_type = fn->return_type
      ? resolver_resolve_type(ctx, fn->return_type) : ctx->builtin_void;

  vec_init_t pvi = {.auto_dispose = false};
  vec_t param_types =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &pvi);

  scope_t saved = ctx->current_scope;
  ctx->current_scope = scope_create(ctx->allocator, ctx->current_scope,
                                     SCOPE_FUNCTION, expr->location);
  vec_push(ctx->all_scopes, ctx->current_scope);

  _check_func_params(ctx, fn, param_types);

  if (fn->body)
    _check_statement(ctx, fn->body, ret_type);

  ctx->current_scope = saved;

  semantic_type_t ftype = semantic_type_create_function(
      ctx->allocator, ret_type, param_types, fn->is_c_variadic);
  type_hash_ensure(ftype);
  vec_push(ctx->all_types, ftype);
  return ftype;
}

static semantic_type_t _check_expr_initialize_list(checker_t ctx, node_t expr) {
  cubec_expression_initialize_list_t il =
      (cubec_expression_initialize_list_t)expr;

  if (il->type) {
    semantic_type_t t = resolver_resolve_type(ctx, il->type);
    if (t->impl->kind == TYPE_ERROR) return ctx->error_type;

    if (t->impl->kind == TYPE_STRUCT || t->impl->kind == TYPE_UNION ||
        t->impl->kind == TYPE_CUNION) {
      vec_t fields = t->impl->struct_type.fields;
      size_t fcount = fields ? vec_get_size(fields) : 0;
      size_t icount = il->items ? vec_get_size(il->items) : 0;
      if (il->is_field)
        _check_init_list_named_fields(ctx, expr, t, fields, fcount, icount,
                                      il->items);
      else
        _check_init_list_positional(ctx, expr, t, fields, fcount, icount,
                                    il->items);
    }
    return t;
  }

  if (il->items && vec_get_size(il->items) > 0 && !il->is_field) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "anonymous initializer list requires explicit type");
    ctx->error_count++;
  } else if (il->is_field) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "named initializer list requires explicit type");
    ctx->error_count++;
  }
  return ctx->error_type;
}

static semantic_type_t _check_expr_comma(checker_t ctx, node_t expr) {
  cubec_expression_comma_t cm = (cubec_expression_comma_t)expr;
  _check_expression(ctx, cm->left);
  return _check_expression(ctx, cm->right);
}

static semantic_type_t _check_expr_spread(checker_t ctx, node_t expr) {
  cubec_expression_spread_t sp = (cubec_expression_spread_t)expr;
  return _check_expression(ctx, sp->value);
}

static semantic_type_t _check_expr_generic_instantiation(checker_t ctx, node_t expr) {
  cubec_expression_generic_instantiation_t gi =
      (cubec_expression_generic_instantiation_t)expr;

  if (gi->callee->kind == CUBEC_NODE_LITERAL_IDENTIFIER)
    return _check_generic_ident_callee(ctx, expr);

  semantic_type_t callee_type = _check_expression(ctx, gi->callee);
  if (gi->arguments) {
    size_t acount = vec_get_size(gi->arguments);
    for (size_t i = 0; i < acount; i++)
      _check_expression(ctx, (node_t)vec_get(gi->arguments, i));
  }
  return callee_type;
}

/* ===== dispatch ===== */

semantic_type_t _check_expression(checker_t ctx, node_t expr) {
  if (!expr) return ctx->error_type;
  switch (expr->kind) {
  case CUBEC_NODE_LITERAL_NUMERIC:       return _check_literal_numeric(ctx, expr);
  case CUBEC_NODE_LITERAL_STRING:        return ctx->builtin_string;
  case CUBEC_NODE_LITERAL_CHAR:          return ctx->builtin_char;
  case CUBEC_NODE_LITERAL_IDENTIFIER:    return _check_expr_literal_identifier(ctx, expr);
  case CUBEC_NODE_EXPRESSION_BINARY:     return _check_expr_binary(ctx, expr);
  case CUBEC_NODE_EXPRESSION_ASSIGNMENT: return _check_expr_assignment(ctx, expr);
  case CUBEC_NODE_EXPRESSION_CALL:       return _check_expr_call(ctx, expr);
  case CUBEC_NODE_EXPRESSION_MEMBER:     return _check_expr_member(ctx, expr);
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS: return _check_expr_namespace_access(ctx, expr);
  case CUBEC_NODE_EXPRESSION_DEREF:      return _check_expr_deref(ctx, expr);
  case CUBEC_NODE_EXPRESSION_ADDR:       return _check_expr_addr(ctx, expr);
  case CUBEC_NODE_EXPRESSION_TRY:        return _check_expr_try(ctx, expr);
  case CUBEC_NODE_EXPRESSION_TERNARY:    return _check_expr_ternary(ctx, expr);
  case CUBEC_NODE_EXPRESSION_GROUP:      return _check_expr_group(ctx, expr);
  case CUBEC_NODE_EXPRESSION_SIZEOF:     return _check_expr_sizeof(ctx, expr);
  case CUBEC_NODE_EXPRESSION_ALIGNOF:    return _check_expr_alignof(ctx, expr);
  case CUBEC_NODE_EXPRESSION_TYPEOF:     return _check_expr_typeof(ctx, expr);
  case CUBEC_NODE_EXPRESSION_SLICE:      return _check_expr_slice(ctx, expr);
  case CUBEC_NODE_EXPRESSION_FUNCTION:   return _check_expr_function(ctx, expr);
  case CUBEC_NODE_EXPRESSION_INITIALIZE_LIST: return _check_expr_initialize_list(ctx, expr);
  case CUBEC_NODE_EXPRESSION_COMMA:      return _check_expr_comma(ctx, expr);
  case CUBEC_NODE_EXPRESSION_SPREAD:     return _check_expr_spread(ctx, expr);
  case CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION: return _check_expr_generic_instantiation(ctx, expr);
  case CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER:
  case CUBEC_NODE_EXPRESSION_TYPE_STRUCT:
  case CUBEC_NODE_EXPRESSION_TYPE_ENUM:
  case CUBEC_NODE_EXPRESSION_TYPE_UNION:
  case CUBEC_NODE_EXPRESSION_TYPE_INTERFACE:
  case CUBEC_NODE_EXPRESSION_TYPE_FUNCTION:
    return resolver_resolve_type(ctx, expr);
  default: return ctx->error_type;
  }
}

semantic_type_t checker_check_expression(checker_t ctx, node_t expr) {
  return _check_expression(ctx, expr);
}
