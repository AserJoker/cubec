#include "engine/checker_check_expr_helpers.h"
#include "engine/checker_check_expr.h"
#include "engine/checker_type_util.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"
#include "cubec/node.h"
#include "cubec/literal_identifier.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/expression_initialize_field.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_function.h"
#include "cubec/function_argument.h"
#include <string.h>

/* ===== assignment generic LHS helper ===== */

semantic_type_t _check_assign_generic_lhs(checker_t ctx, node_t expr,
                                           semantic_type_t lt,
                                           semantic_type_t rt) {
  cubec_expression_assignment_t asgn = (cubec_expression_assignment_t)expr;
  cubec_expression_generic_instantiation_t gi =
      (cubec_expression_generic_instantiation_t)asgn->left;
  if (gi->callee->kind != CUBEC_NODE_LITERAL_IDENTIFIER)
    return NULL;

  const char *name = _checker_ident_str(gi->callee);
  struct symbol *sym = name ? scope_lookup(ctx->current_scope, name) : NULL;
  if (!sym || sym->kind != SYMBOL_VARIABLE || !sym->variable.type)
    return NULL;

  semantic_type_t host_type = sym->variable.type;
  if (host_type->instance_methods) {
    size_t mcount = vec_get_size(host_type->instance_methods);
    for (size_t i = 0; i < mcount; i++) {
      struct symbol *m = (struct symbol *)vec_get(host_type->instance_methods, i);
      if (m && m->name && strcmp(m->name, "__set__") == 0 && m->function.type) {
        vec_t params = m->function.type->impl->function.params;
        if (params && vec_get_size(params) >= 2) {
          semantic_type_t value_param =
              (semantic_type_t)vec_get(params, vec_get_size(params) - 1);
          if (!semantic_type_can_implicit_convert(rt, value_param)) {
            diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                                 expr->location,
                                 "cannot assign '%s' to __set__ value parameter '%s'",
                                 rt->name ? rt->name : "<anonymous>",
                                 value_param->name ? value_param->name : "<anonymous>");
            ctx->error_count++;
          }
        }
        return ctx->builtin_void;
      }
    }
  }
  if (host_type->impl->kind == TYPE_ARRAY || host_type->impl->kind == TYPE_SLICE) {
    if (!semantic_type_can_implicit_convert(rt, lt)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                           "cannot assign '%s' to '%s'",
                           rt->name ? rt->name : "<anonymous>",
                           lt->name ? lt->name : "<anonymous>");
      ctx->error_count++;
    }
    return lt;
  }
  return NULL;
}

/* ===== generic instantiation ident callee helper ===== */

semantic_type_t _check_generic_ident_callee(checker_t ctx, node_t expr) {
  cubec_expression_generic_instantiation_t gi =
      (cubec_expression_generic_instantiation_t)expr;
  const char *name = _checker_ident_str(gi->callee);
  struct symbol *sym = name ? scope_lookup(ctx->current_scope, name) : NULL;

  if (sym && sym->kind == SYMBOL_TYPE && sym->type.type) {
    semantic_type_t template_type = sym->type.type;
    vec_t type_args = _resolve_generic_type_args(ctx, gi->arguments);
    if (!type_args) return ctx->error_type;
    if (template_type->impl->kind == TYPE_GENERIC_INSTANCE) return template_type;
    (void)type_args;
    return _instantiate_type(ctx, template_type, type_args, expr);
  }

  if (sym && sym->kind == SYMBOL_FUNCTION && sym->function.type) {
    vec_t type_args = _resolve_generic_type_args(ctx, gi->arguments);
    if (!type_args) return ctx->error_type;
    return _instantiate_function(ctx, sym, type_args, expr);
  }

  /* Subscript indexing on value */
  semantic_type_t host_type = _check_expression(ctx, gi->callee);
  if (host_type->impl->kind == TYPE_ERROR) return ctx->error_type;

  if (gi->arguments && vec_get_size(gi->arguments) >= 1)
    _check_expression(ctx, (node_t)vec_get(gi->arguments, 0));

  if (host_type->impl->kind == TYPE_ARRAY)
    return host_type->impl->array.element;
  if (host_type->impl->kind == TYPE_SLICE)
    return host_type->impl->slice.element;

  if (host_type->instance_methods) {
    size_t mcount = vec_get_size(host_type->instance_methods);
    for (size_t i = 0; i < mcount; i++) {
      struct symbol *m = (struct symbol *)vec_get(host_type->instance_methods, i);
      if (m && m->name && strcmp(m->name, "__get__") == 0 && m->function.type)
        return m->function.type->impl->function.return_type;
    }
  }

  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                       "type does not support indexing");
  ctx->error_count++;
  return ctx->error_type;
}

/* ===== init list helpers ===== */

void _check_init_list_named_fields(checker_t ctx, node_t expr,
                                    semantic_type_t t, vec_t fields,
                                    size_t fcount, size_t icount,
                                    vec_t items) {
  for (size_t i = 0; i < icount; i++) {
    node_t item = (node_t)vec_get(items, i);
    if (item->kind != CUBEC_NODE_EXPRESSION_INITIALIZE_FIELD) continue;
    cubec_expression_initialize_field_t f =
        (cubec_expression_initialize_field_t)item;
    const char *fname = _checker_ident_str((node_t)f->field);
    struct symbol *fsym = NULL;
    for (size_t j = 0; j < fcount; j++) {
      struct symbol *s = (struct symbol *)vec_get(fields, j);
      if (s && s->name && strcmp(s->name, fname) == 0) {
        fsym = s;
        break;
      }
    }
    if (!fsym) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           item->location, "type '%s' has no field '%s'",
                           t->name ? t->name : "<anonymous>", fname);
      ctx->error_count++;
    } else if (f->value) {
      semantic_type_t vt = _check_expression(ctx, f->value);
      if (vt->impl->kind != TYPE_ERROR &&
          !semantic_type_can_implicit_convert(vt, fsym->field.type)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             item->location,
                             "cannot initialize field '%s' of type '%s' with '%s'",
                             fname,
                             fsym->field.type->name
                                 ? fsym->field.type->name : "<anonymous>",
                             vt->name ? vt->name : "<anonymous>");
        ctx->error_count++;
      }
    }
  }
}

void _check_init_list_positional(checker_t ctx, node_t expr,
                                  semantic_type_t t, vec_t fields,
                                  size_t fcount, size_t icount,
                                  vec_t items) {
  for (size_t i = 0; i < icount && i < fcount; i++) {
    node_t item = (node_t)vec_get(items, i);
    struct symbol *fsym = (struct symbol *)vec_get(fields, i);
    if (item && fsym && fsym->field.type) {
      semantic_type_t vt = _check_expression(ctx, item);
      if (vt->impl->kind != TYPE_ERROR &&
          !semantic_type_can_implicit_convert(vt, fsym->field.type)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             item->location,
                             "cannot initialize field '%s' of type '%s' with '%s'",
                             fsym->name ? fsym->name : "<anonymous>",
                             fsym->field.type->name
                                 ? fsym->field.type->name : "<anonymous>",
                             vt->name ? vt->name : "<anonymous>");
        ctx->error_count++;
      }
    }
  }
  if (icount > fcount) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         expr->location,
                         "too many initializers for type '%s'",
                         t->name ? t->name : "<anonymous>");
    ctx->error_count++;
  }
}

/* ===== binary operator helpers ===== */

semantic_type_t _check_binary_arithmetic(checker_t ctx, node_t expr,
                                          const char *op,
                                          semantic_type_t lt,
                                          semantic_type_t rt) {
  if (!_is_numeric_type(lt) || !_is_numeric_type(rt)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "arithmetic operator '%s' requires numeric operands", op);
    ctx->error_count++;
    return ctx->error_type;
  }
  return _common_type(ctx, lt, rt);
}

semantic_type_t _check_binary_bitwise(checker_t ctx, node_t expr,
                                       const char *op,
                                       semantic_type_t lt,
                                       semantic_type_t rt) {
  if (!_is_integer_type(lt) || !_is_integer_type(rt)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "bitwise operator '%s' requires integer operands", op);
    ctx->error_count++;
    return ctx->error_type;
  }
  return _common_type(ctx, lt, rt);
}

bool _is_op_one_of(const char *op, const char **ops, size_t count) {
  for (size_t i = 0; i < count; i++) {
    if (strcmp(op, ops[i]) == 0) return true;
  }
  return false;
}

/* ===== function parameter helper ===== */

void _check_func_params(checker_t ctx, void *fn_node, vec_t param_types) {
  cubec_expression_function_t fn = (cubec_expression_function_t)fn_node;
  if (!fn->arguments) return;
  size_t acount = vec_get_size(fn->arguments);
  for (size_t i = 0; i < acount; i++) {
    node_t arg = (node_t)vec_get(fn->arguments, i);
    if (arg->kind != CUBEC_NODE_FUNCTION_ARGUMENT) continue;
    cubec_function_argument_t farg = (cubec_function_argument_t)arg;
    semantic_type_t pt = farg->type
        ? resolver_resolve_type(ctx, farg->type) : ctx->error_type;
    vec_push(param_types, pt);
    const char *pname = _checker_ident_str(farg->identifier);
    if (pname) {
      struct symbol *psym = symbol_create(ctx->allocator, pname,
                                           SYMBOL_VARIABLE, arg->location);
      psym->variable.type = pt;
      psym->variable.is_mutable = true;
      psym->state = SYMBOL_EVALUATED;
      scope_push_symbol(ctx->current_scope, psym);
    }
  }
}
