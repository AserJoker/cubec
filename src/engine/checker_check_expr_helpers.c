#include "engine/checker_check_expr_helpers.h"
#include "engine/checker_check_expr.h"
#include "engine/checker_check_stmt.h"
#include "engine/checker_type_util.h"
#include "engine/resolver.h"
#include "engine/type_hash.h"
#include "cubec/node.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/expression_initialize_field.h"
#include "cubec/expression_spread.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_function.h"
#include "cubec/function_argument.h"
#include "cubec/generic_param.h"
#include <string.h>

/* ===== assignment generic LHS helper ===== */

semantic_type_t _check_assign_generic_lhs(context_t ctx, node_t expr,
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

  /* str[index] = char: compile-time writable */
  if (host_type->impl->kind == TYPE_STR) {
    if (!semantic_type_can_implicit_convert(rt, ctx->builtin_char)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
          "cannot assign '%s' to str index (expected char)",
          rt->name ? rt->name : "<anonymous>");
      ctx->error_count++;
    }
    return ctx->builtin_void;
  }

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

semantic_type_t _check_generic_ident_callee(context_t ctx, node_t expr) {
  cubec_expression_generic_instantiation_t gi =
      (cubec_expression_generic_instantiation_t)expr;
  const char *name = _checker_ident_str(gi->callee);
  struct symbol *sym = name ? scope_lookup(ctx->current_scope, name) : NULL;

  if (sym && sym->kind == SYMBOL_TYPE && sym->type.type) {
    semantic_type_t template_type = sym->type.type;
    if (template_type->impl->kind == TYPE_GENERIC_INSTANCE) {
      strmap_t dummy = _resolve_generic_type_bindings_pack(ctx, gi->arguments, sym->type.generic_params);
      allocator_free(ctx->allocator, &dummy);
      return template_type;
    }

    strmap_t type_bindings = _resolve_generic_type_bindings_pack(ctx, gi->arguments, sym->type.generic_params);
    if (strmap_get_size(type_bindings) == 0 && vec_get_size(gi->arguments) > 0) {
      allocator_free(ctx->allocator, &type_bindings);
      return ctx->error_type;
    }
    _check_generic_param_constraints(ctx, sym->type.generic_params, type_bindings, expr);
    return _instantiate_type(ctx, template_type, type_bindings, expr);
  }

  if (sym && sym->kind == SYMBOL_FUNCTION && sym->function.type) {
    strmap_t type_bindings_fn = _resolve_generic_type_bindings_pack(ctx,
        gi->arguments, sym->function.generic_params);
    if (strmap_get_size(type_bindings_fn) == 0 && vec_get_size(gi->arguments) > 0) {
      allocator_free(ctx->allocator, &type_bindings_fn);
      return ctx->error_type;
    }

    /* Check if all generic params are resolved — if not, return the template
       type so _check_expr_call's generic_func_sym path can infer remaining
       params from call arguments (e.g. unionIs[T,K] where K is inferred). */
    vec_t gp = sym->function.generic_params;
    size_t gp_count = gp ? vec_get_size(gp) : 0;
    bool all_bound = true;
    for (size_t i = 0; i < gp_count; i++) {
      cubec_generic_param_t gp_node = (cubec_generic_param_t)(void *)vec_get(gp, i);
      const char *gp_name = gp_node ? _checker_ident_str(gp_node->name) : NULL;
      if (gp_name && !strmap_find(type_bindings_fn, gp_name)) {
        /* Unbound pack param with no explicit args → empty expansion is valid */
        if (gp_node && gp_node->is_rest &&
            (!gi->arguments || vec_get_size(gi->arguments) == 0)) {
          semantic_type_t empty_pack = semantic_type_create_generic_pack(
              ctx->allocator, gp_name);
          type_hash_ensure(empty_pack);
          vec_push(ctx->all_types, empty_pack);
          strmap_insert(type_bindings_fn, gp_name, empty_pack);
          continue;
        }
        all_bound = false;
      }
    }

    if (!all_bound) {
      /* Not all generic params are explicitly provided — return template type.
         The caller (_check_expr_call) will set generic_func_sym and use
         _infer_type_args_from_call to resolve the remaining params. */
      allocator_free(ctx->allocator, &type_bindings_fn);
      return sym->function.type;
    }

    _check_generic_param_constraints(ctx, sym->function.generic_params, type_bindings_fn, expr);
    semantic_type_t inst_result = _instantiate_function(ctx, sym, type_bindings_fn, expr);
    if (inst_result->impl->kind != TYPE_ERROR) {
      /* Enqueue for body checking — _enqueue_body_check takes ownership
         of type_bindings on success, frees on duplicate. */
      _enqueue_body_check(ctx, sym, inst_result, type_bindings_fn,
          ctx->global_scope, false, NULL);
      type_bindings_fn = NULL; /* ownership transferred */
    }
    if (type_bindings_fn) allocator_free(ctx->allocator, &type_bindings_fn);
    return inst_result;
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
  if (host_type->impl->kind == TYPE_STR)
    return ctx->builtin_char;

  /* Tuple subscript: callee is a tuple-typed variable */
  if (host_type->impl->kind == TYPE_TUPLE ||
      (host_type->impl->kind == TYPE_GENERIC_INSTANCE &&
       host_type->impl->generic_instance.fields)) {
    if (gi->arguments && vec_get_size(gi->arguments) == 1) {
      node_t idx_node = (node_t)vec_get(gi->arguments, 0);
      uint64_t idx = 0;
      bool idx_valid = false;
      if (idx_node->kind == CUBEC_NODE_LITERAL_NUMERIC) {
        cubec_literal_numeric_t num = (cubec_literal_numeric_t)idx_node;
        idx = strtoull(string_get(num->value), NULL, 10);
        idx_valid = true;
      }
      if (!idx_valid) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                             "tuple subscript index must be a compile-time constant");
        ctx->error_count++;
        return ctx->error_type;
      }
      vec_t fields = host_type->impl->kind == TYPE_TUPLE
          ? host_type->impl->tuple.fields
          : host_type->impl->generic_instance.fields;
      size_t fcount = fields ? vec_get_size(fields) : 0;
      if (idx >= fcount) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                             "tuple index %llu out of range (tuple has %zu element%s)",
                             (unsigned long long)idx, fcount, fcount == 1 ? "" : "s");
        ctx->error_count++;
        return ctx->error_type;
      }
      struct symbol *field = (struct symbol *)vec_get(fields, (size_t)idx);
      return field ? field->field.type : ctx->error_type;
    }
  }

  if (host_type->instance_methods) {
    size_t mcount = vec_get_size(host_type->instance_methods);
    struct symbol *set_method = NULL;
    for (size_t i = 0; i < mcount; i++) {
      struct symbol *m = (struct symbol *)vec_get(host_type->instance_methods, i);
      if (m && m->name && strcmp(m->name, "__get__") == 0 && m->function.type)
        return m->function.type->impl->function.return_type;
      if (m && m->name && strcmp(m->name, "__set__") == 0 && m->function.type)
        set_method = m;
    }
    /* If __set__ exists (even without __get__), the type supports indexing.
       Return the value parameter type of __set__ so assignment type-checking works. */
    if (set_method) {
      vec_t params = set_method->function.type->impl->function.params;
      if (params && vec_get_size(params) >= 2)
        return (semantic_type_t)vec_get(params, vec_get_size(params) - 1);
      return ctx->builtin_void;
    }
  }

  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                       "type does not support indexing");
  ctx->error_count++;
  return ctx->error_type;
}

/* ===== init list helpers ===== */

void _check_init_list_named_fields(context_t ctx, node_t expr,
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

void _check_init_list_positional(context_t ctx, node_t expr,
                                  semantic_type_t t, vec_t fields,
                                  size_t fcount, size_t icount,
                                  vec_t items) {
  size_t field_idx = 0;
  for (size_t i = 0; i < icount; i++) {
    node_t item = (node_t)vec_get(items, i);

    if (item && item->kind == CUBEC_NODE_EXPRESSION_SPREAD) {
      /* Pack spread: evaluate the spread expression and check expanded types */
      semantic_type_t spread_type = _check_expression(ctx, item);
      if (spread_type && spread_type->impl && spread_type->impl->kind == TYPE_GENERIC_PACK) {
        vec_t expanded = spread_type->impl->generic_pack.expanded_types;
        size_t ecount = expanded ? vec_get_size(expanded) : 0;
        for (size_t j = 0; j < ecount && field_idx < fcount; j++, field_idx++) {
          semantic_type_t et = (semantic_type_t)vec_get(expanded, j);
          struct symbol *fsym = (struct symbol *)vec_get(fields, field_idx);
          if (et && fsym && fsym->field.type) {
            if (et->impl->kind != TYPE_ERROR &&
                !semantic_type_can_implicit_convert(et, fsym->field.type)) {
              diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                                   item->location,
                                   "cannot initialize field '%s' of type '%s' with '%s'",
                                   fsym->name ? fsym->name : "<anonymous>",
                                   fsym->field.type->name
                                       ? fsym->field.type->name : "<anonymous>",
                                   et->name ? et->name : "<anonymous>");
              ctx->error_count++;
            }
          }
        }
      }
      /* If spread_type is not TYPE_GENERIC_PACK, it's already been type-checked
         by _check_expression → _check_expr_spread, which returns the inner type.
         In that case, we skip expansion and treat it as a single element. */
    } else {
      /* Regular positional item */
      if (field_idx < fcount) {
        struct symbol *fsym = (struct symbol *)vec_get(fields, field_idx);
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
        field_idx++;
      }
    }
  }
  if (field_idx > fcount) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         expr->location,
                         "too many initializers for type '%s'",
                         t->name ? t->name : "<anonymous>");
    ctx->error_count++;
  }
}

/* ===== binary operator helpers ===== */

semantic_type_t _check_binary_arithmetic(context_t ctx, node_t expr,
                                          const char *op,
                                          semantic_type_t lt,
                                          semantic_type_t rt) {
  /* __value__ fallback: unwrap struct-like operands that have __value__ */
  semantic_type_t effective_lt = lt;
  semantic_type_t effective_rt = rt;
  if (!_is_numeric_type(lt)) {
    semantic_type_t lt_unq = semantic_type_strip_qualifier(lt);
    bool lt_struct = lt_unq->impl->kind == TYPE_STRUCT ||
                     lt_unq->impl->kind == TYPE_UNION ||
                     lt_unq->impl->kind == TYPE_CUNION ||
                     lt_unq->impl->kind == TYPE_GENERIC_INSTANCE;
    if (lt_struct && lt->instance_methods) {
      size_t mc = vec_get_size(lt->instance_methods);
      for (size_t i = 0; i < mc; i++) {
        struct symbol *s = (struct symbol *)vec_get(lt->instance_methods, i);
        if (s && s->name && strcmp(s->name, "__value__") == 0 &&
            s->kind == SYMBOL_FUNCTION && s->function.type) {
          effective_lt = s->function.type->impl->function.return_type;
          break;
        }
      }
    }
  }
  if (!_is_numeric_type(rt)) {
    semantic_type_t rt_unq = semantic_type_strip_qualifier(rt);
    bool rt_struct = rt_unq->impl->kind == TYPE_STRUCT ||
                     rt_unq->impl->kind == TYPE_UNION ||
                     rt_unq->impl->kind == TYPE_CUNION ||
                     rt_unq->impl->kind == TYPE_GENERIC_INSTANCE;
    if (rt_struct && rt->instance_methods) {
      size_t mc = vec_get_size(rt->instance_methods);
      for (size_t i = 0; i < mc; i++) {
        struct symbol *s = (struct symbol *)vec_get(rt->instance_methods, i);
        if (s && s->name && strcmp(s->name, "__value__") == 0 &&
            s->kind == SYMBOL_FUNCTION && s->function.type) {
          effective_rt = s->function.type->impl->function.return_type;
          break;
        }
      }
    }
  }

  if (!_is_numeric_type(effective_lt) || !_is_numeric_type(effective_rt)) {
    /* str + str → str (concatenation) */
    if (op[0] == '+' && op[1] == '\0') {
      semantic_type_t lt_unq = semantic_type_strip_qualifier(lt);
      semantic_type_t rt_unq = semantic_type_strip_qualifier(rt);
      if (lt_unq->impl->kind == TYPE_STR && rt_unq->impl->kind == TYPE_STR)
        return ctx->builtin_str;
    }
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "arithmetic operator '%s' requires numeric operands", op);
    ctx->error_count++;
    return ctx->error_type;
  }
  return _common_type(ctx, effective_lt, effective_rt);
}

semantic_type_t _check_binary_bitwise(context_t ctx, node_t expr,
                                       const char *op,
                                       semantic_type_t lt,
                                       semantic_type_t rt) {
  /* __value__ fallback: unwrap struct-like operands that have __value__ */
  semantic_type_t effective_lt = lt;
  semantic_type_t effective_rt = rt;
  if (!_is_integer_type(lt)) {
    semantic_type_t lt_unq = semantic_type_strip_qualifier(lt);
    bool lt_struct = lt_unq->impl->kind == TYPE_STRUCT ||
                     lt_unq->impl->kind == TYPE_UNION ||
                     lt_unq->impl->kind == TYPE_CUNION ||
                     lt_unq->impl->kind == TYPE_GENERIC_INSTANCE;
    if (lt_struct && lt->instance_methods) {
      size_t mc = vec_get_size(lt->instance_methods);
      for (size_t i = 0; i < mc; i++) {
        struct symbol *s = (struct symbol *)vec_get(lt->instance_methods, i);
        if (s && s->name && strcmp(s->name, "__value__") == 0 &&
            s->kind == SYMBOL_FUNCTION && s->function.type) {
          effective_lt = s->function.type->impl->function.return_type;
          break;
        }
      }
    }
  }
  if (!_is_integer_type(rt)) {
    semantic_type_t rt_unq = semantic_type_strip_qualifier(rt);
    bool rt_struct = rt_unq->impl->kind == TYPE_STRUCT ||
                     rt_unq->impl->kind == TYPE_UNION ||
                     rt_unq->impl->kind == TYPE_CUNION ||
                     rt_unq->impl->kind == TYPE_GENERIC_INSTANCE;
    if (rt_struct && rt->instance_methods) {
      size_t mc = vec_get_size(rt->instance_methods);
      for (size_t i = 0; i < mc; i++) {
        struct symbol *s = (struct symbol *)vec_get(rt->instance_methods, i);
        if (s && s->name && strcmp(s->name, "__value__") == 0 &&
            s->kind == SYMBOL_FUNCTION && s->function.type) {
          effective_rt = s->function.type->impl->function.return_type;
          break;
        }
      }
    }
  }

  if (!_is_integer_type(effective_lt) || !_is_integer_type(effective_rt)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "bitwise operator '%s' requires integer operands", op);
    ctx->error_count++;
    return ctx->error_type;
  }
  return _common_type(ctx, effective_lt, effective_rt);
}

bool _is_op_one_of(const char *op, const char **ops, size_t count) {
  for (size_t i = 0; i < count; i++) {
    if (strcmp(op, ops[i]) == 0) return true;
  }
  return false;
}

/* _check_func_params replaced by _resolve_func_param_types + _register_func_params_from_info
   in checker_func_util.c */
