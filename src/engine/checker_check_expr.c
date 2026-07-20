#include "engine/checker.h"
#include "engine/checker_check_expr.h"
#include "engine/checker_check_expr_helpers.h"
#include "engine/checker_func_util.h"
#include "engine/checker_type_util.h"
#include "engine/checker_check_stmt.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
#include "engine/builtin.h"
#include "engine/comptime_value.h"
#include "engine/flow_state.h"
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
#include "cubec/expression_initialize_field.h"
#include "cubec/generic_param.h"
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

  /* Boolean literals are syntax, not identifiers */
  if (strcmp(name, "true") == 0 || strcmp(name, "false") == 0)
    return ctx->builtin_bool;

  struct symbol *sym = scope_lookup(ctx->current_scope, name);
  if (!sym) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "undeclared identifier '%s'", name);
    ctx->error_count++;
    return ctx->error_type;
  }
  switch (sym->kind) {
  case SYMBOL_VARIABLE: {
    /* TDZ check: variable initialized with `= undefined` is in TDZ state.
     * Any use is an error unless the variable has been assigned on the
     * current path (tracked by flow_state->tdz_set removal on assignment). */
    if (sym->state == SYMBOL_TDZ) {
      if (!ctx->current_flow || flow_state_is_tdz(ctx->current_flow, name)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                             "use of variable '%s' before initialization", name);
        ctx->error_count++;
        return ctx->error_type;
      }
    }
    return sym->variable.type ? sym->variable.type : ctx->error_type;
  }
  case SYMBOL_FUNCTION: return sym->function.type ? sym->function.type : ctx->error_type;
  case SYMBOL_ENUM_ITEM: return sym->enum_item.owning_type;
  case SYMBOL_TYPE: return sym->type.type ? sym->type.type : ctx->error_type;
  case SYMBOL_GENERIC_PARAM:
    if (sym->generic_param.value_type)
      return sym->generic_param.value_type;
    if (sym->generic_param.constraint)
      return sym->generic_param.constraint;
    return ctx->error_type;
  default: return ctx->error_type;
  }
}

static semantic_type_t _check_expr_literal_undefined(checker_t ctx, node_t expr) {
  /* undefined is only valid as a variable initializer (handled in
   * _check_stmt_declaration).  Using it as a standalone expression
   * is an error — it has no type of its own. */
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                       "'undefined' is not valid as a standalone expression");
  ctx->error_count++;
  return ctx->error_type;
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
  if (_is_op_one_of(op, cmp_ops, 6)) {
    /* __value__ fallback for comparison operands (struct-like only) */
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
    if (!_is_comparable_type(effective_lt) || !_is_comparable_type(effective_rt)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                           "comparison operator '%s' requires comparable operands", op);
      ctx->error_count++;
    }
    return ctx->builtin_bool;
  }
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

/* Check if an lvalue expression refers to a const-qualified location */
static bool _is_const_lvalue(checker_t ctx, node_t expr, semantic_type_t expr_type) {
  if (!expr || !expr_type) return false;

  switch (expr->kind) {
  case CUBEC_NODE_LITERAL_IDENTIFIER:
    return semantic_type_is_const(expr_type);

  case CUBEC_NODE_EXPRESSION_MEMBER: {
    cubec_expression_member_t mem = (cubec_expression_member_t)expr;
    semantic_type_t host_type = _check_expression(ctx, mem->host);
    /* If host is const, the field is const */
    if (semantic_type_is_const(host_type)) return true;
    /* For pointer.field, check if pointee is const */
    semantic_type_t host_unq = semantic_type_strip_qualifier(host_type);
    if (host_unq->impl->kind == TYPE_POINTER) {
      semantic_type_t pointee = host_unq->impl->pointer.pointee;
      if (semantic_type_is_const(pointee)) return true;
    }
    return semantic_type_is_const(expr_type);
  }

  case CUBEC_NODE_EXPRESSION_DEREF: {
    cubec_expression_binary_t deref = (cubec_expression_binary_t)expr;
    semantic_type_t ptr_type = _check_expression(ctx, deref->right);
    semantic_type_t ptr_unq = semantic_type_strip_qualifier(ptr_type);
    if (ptr_unq->impl->kind == TYPE_POINTER) {
      semantic_type_t pointee = ptr_unq->impl->pointer.pointee;
      if (semantic_type_is_const(pointee)) return true;
    }
    return semantic_type_is_const(expr_type);
  }

  default:
    return semantic_type_is_const(expr_type);
  }
}

static semantic_type_t _check_expr_assignment(checker_t ctx, node_t expr) {
  cubec_expression_assignment_t asgn = (cubec_expression_assignment_t)expr;
  const char *op = string_get(asgn->opt);

  /* Wildcard discard: _ = expr — always valid, no lvalue/const checks */
  if (asgn->left && asgn->left->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
    const char *lname = _checker_ident_str(asgn->left);
    if (lname && lname[0] == '_' && lname[1] == '\0') {
      semantic_type_t rt = _check_expression(ctx, asgn->right);
      return rt;
    }
  }

  if (!_is_lvalue(asgn->left)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "left side of assignment is not a valid lvalue");
    ctx->error_count++;
  }

  /* For TDZ variables being assigned, remove from tdz_set before checking
   * the lvalue — the assignment itself eliminates the TDZ state. */
  if (asgn->left && asgn->left->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
    const char *lname = _checker_ident_str(asgn->left);
    if (lname && ctx->current_flow && flow_state_is_tdz(ctx->current_flow, lname)) {
      flow_state_remove_tdz(ctx->current_flow, lname);
    }
  }

  semantic_type_t lt = _check_expression(ctx, asgn->left);
  semantic_type_t rt = _check_expression(ctx, asgn->right);

  /* For subscript LHS with __set__, check before early error return.
     When __set__ exists but __get__ doesn't, the _check_expression call above
     will have emitted "type does not support indexing" and returned error_type.
     We still need to validate __set__ for the assignment. */
  if (asgn->left->kind == CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION) {
    semantic_type_t result = _check_assign_generic_lhs(ctx, expr, lt, rt);
    if (result) return result;
  }

  if (lt->impl->kind == TYPE_ERROR || rt->impl->kind == TYPE_ERROR)
    return ctx->error_type;

  /* Check const enforcement: cannot assign to const-qualified lvalue */
  if (_is_const_lvalue(ctx, asgn->left, lt)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "cannot assign to const-qualified expression");
    ctx->error_count++;
  }

  if (strcmp(op, "=") == 0) {
    if (!semantic_type_can_implicit_convert(rt, lt)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                           "cannot assign '%s' to '%s'",
                           rt->name ? rt->name : "<anonymous>",
                           lt->name ? lt->name : "<anonymous>");
      ctx->error_count++;
    }
    return ctx->builtin_void;
  }

  if (!semantic_type_can_implicit_convert(rt, lt)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "type mismatch in compound assignment");
    ctx->error_count++;
  }
  return ctx->builtin_void;
}

static semantic_type_t _check_expr_call(checker_t ctx, node_t expr) {
  cubec_expression_call_t call = (cubec_expression_call_t)expr;

  /* Check if callee is a direct reference to a generic function (needs type inference) */
  struct symbol *generic_func_sym = NULL;
  vec_t explicit_type_args = NULL;

  if (call->callee->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
    const char *name = _checker_ident_str(call->callee);
    struct symbol *sym = name ? scope_lookup(ctx->current_scope, name) : NULL;
    if (sym && sym->kind == SYMBOL_FUNCTION && sym->function.generic_params)
      generic_func_sym = sym;
  } else if (call->callee->kind == CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION) {
    /* Handle builtin generic instantiation callees like getTupleItem[0](t) */
    cubec_expression_generic_instantiation_t gi =
        (cubec_expression_generic_instantiation_t)call->callee;
    if (gi->callee->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
      const char *name = _checker_ident_str(gi->callee);
      struct symbol *sym = name ? scope_lookup(ctx->current_scope, name) : NULL;
      if (sym && sym->kind == SYMBOL_FUNCTION && sym->function.generic_params
          && sym->is_builtin) {
        generic_func_sym = sym;
        /* Resolve explicit type args from the generic_instantiation AST node */
        explicit_type_args = _resolve_generic_type_args(ctx, gi->arguments,
            sym->function.generic_params);
        /* Coalesce excess type args into packs for generic functions with rest params.
           E.g. for getTupleItem[N, ...Args], getTupleItem[0, i32, f64] should produce
           explicit_type_args = [TYPE_GENERIC_VALUE(0), PACK([i32, f64])],
           not [TYPE_GENERIC_VALUE(0), i32, f64]. */
        if (explicit_type_args) {
          vec_t gp = sym->function.generic_params;
          size_t gcount = gp ? vec_get_size(gp) : 0;
          size_t tacount = vec_get_size(explicit_type_args);
          /* Find the pack parameter position */
          size_t pack_idx = gcount;
          for (size_t i = 0; i < gcount; i++) {
            cubec_generic_param_t gp_node = (cubec_generic_param_t)(void *)vec_get(gp, i);
            if (gp_node && gp_node->is_rest) {
              pack_idx = i;
              break;
            }
          }
          if (pack_idx < gcount && tacount > pack_idx) {
            /* If all args from pack_idx onward form exactly one TYPE_GENERIC_PACK,
               use it directly instead of re-wrapping. */
            if (pack_idx + 1 == tacount) {
              semantic_type_t only_arg = (semantic_type_t)vec_get(explicit_type_args, pack_idx);
              if (only_arg && only_arg->impl->kind == TYPE_GENERIC_PACK) {
                /* Already a pack — no coalescing needed */
              } else {
                /* Single non-pack arg at pack position or multiple args — coalesce */
                goto do_coalesce_func;
              }
            } else {
            do_coalesce_func:;
              vec_init_t cvi = {.auto_dispose = false};
              vec_t new_type_args = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &cvi);
              for (size_t i = 0; i < pack_idx; i++)
                vec_push(new_type_args, vec_get(explicit_type_args, i));
              const char *pack_name = NULL;
              cubec_generic_param_t pack_gp = (cubec_generic_param_t)(void *)vec_get(gp, pack_idx);
              if (pack_gp) {
                const char *raw = _checker_ident_str(pack_gp->name);
                if (raw) pack_name = raw;
              }
              semantic_type_t pack_type = semantic_type_create_generic_pack(
                  ctx->allocator, pack_name, pack_idx);
              for (size_t i = pack_idx; i < tacount; i++) {
                semantic_type_t ta = (semantic_type_t)vec_get(explicit_type_args, i);
                vec_push(pack_type->impl->generic_pack.expanded_types, ta);
              }
              type_hash_ensure(pack_type);
              vec_push(ctx->all_types, pack_type);
              vec_push(new_type_args, pack_type);
              allocator_free(ctx->allocator, &explicit_type_args);
              explicit_type_args = new_type_args;
            }
          }
        }
      }
    }
  }

  semantic_type_t callee_type = _check_expression(ctx, call->callee);
  if (callee_type->impl->kind == TYPE_ERROR) return ctx->error_type;

  if (callee_type->impl->kind != TYPE_FUNCTION) {
    /* Check for __call__ magic method */
    struct symbol *call_method = NULL;
    if (callee_type->instance_methods) {
      size_t mc = vec_get_size(callee_type->instance_methods);
      for (size_t i = 0; i < mc; i++) {
        struct symbol *s = (struct symbol *)vec_get(callee_type->instance_methods, i);
        if (s && s->name && strcmp(s->name, "__call__") == 0 &&
            s->kind == SYMBOL_FUNCTION && s->function.type) {
          call_method = s;
          break;
        }
      }
    }
    if (call_method) {
      semantic_type_t call_fn_type = call_method->function.type;
      vec_t params = call_fn_type->impl->function.params;
      size_t param_count = params ? vec_get_size(params) : 0;
      /* Skip first param (self); check remaining against user args */
      size_t user_arg_count = call->arguments ? vec_get_size(call->arguments) : 0;
      for (size_t i = 0; i < user_arg_count; i++) {
        node_t arg = (node_t)vec_get(call->arguments, i);
        semantic_type_t at = _check_expression(ctx, arg);
        size_t pidx = i + 1; /* offset by self param */
        if (pidx < param_count && at->impl->kind != TYPE_ERROR) {
          semantic_type_t pt = (semantic_type_t)vec_get(params, pidx);
          if (!semantic_type_can_implicit_convert(at, pt)) {
            diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg->location,
                "argument %zu: cannot convert '%s' to '%s'",
                i + 1,
                at->name ? at->name : "<anonymous>",
                pt->name ? pt->name : "<anonymous>");
            ctx->error_count++;
          }
        }
      }
      return call_fn_type->impl->function.return_type;
    }

    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "call of non-function type");
    ctx->error_count++;
    return ctx->error_type;
  }

  size_t arg_count = call->arguments ? vec_get_size(call->arguments) : 0;

  /* Generic function inference: infer type args from call arguments */
  if (generic_func_sym) {
    /* Collect argument types */
    vec_init_t atvi = {.auto_dispose = false};
    vec_t arg_types = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &atvi);
    for (size_t i = 0; i < arg_count; i++) {
      node_t arg = (node_t)vec_get(call->arguments, i);
      semantic_type_t at = _check_expression(ctx, arg);
      vec_push(arg_types, at);
    }

    /* Build generic param info for _infer_type_args_from_call.
       We store the generic param names (const char*) directly so that
       _infer_type_args_from_call can look up bindings by name.
       Generic param names are NOT in scope (they are template-level),
       so scope_lookup won't find them — we extract names from AST nodes. */
    vec_t gp = generic_func_sym->function.generic_params;
    size_t gcount = vec_get_size(gp);
    vec_init_t gpvi = {.auto_dispose = false};
    vec_t generic_param_names = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &gpvi);
    for (size_t i = 0; i < gcount; i++) {
      cubec_generic_param_t gp_node = (cubec_generic_param_t)(void *)vec_get(gp, i);
      const char *gp_name = gp_node ? _checker_ident_str(gp_node->name) : NULL;
      vec_push(generic_param_names, (void *)gp_name);
    }

    /* Infer type args — use the template type for inference, not the
       potentially partially-instantiated callee_type (e.g. when the callee
       is a generic_instantiation like getTupleItem[0]). */
    vec_t type_args = _infer_type_args_from_call(ctx,
        generic_func_sym->function.type,
        generic_param_names, arg_types, explicit_type_args);

    /* Check for unresolved generic params */
    if (type_args) {
      size_t tcount = vec_get_size(type_args);
      for (size_t i = 0; i < tcount; i++) {
        semantic_type_t ta = (semantic_type_t)vec_get(type_args, i);
        if (!ta) {
          cubec_generic_param_t gp_node = (cubec_generic_param_t)(void *)vec_get(gp, i);
          const char *gp_name = gp_node ? _checker_ident_str(gp_node->name) : "?";
          diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                               expr->location,
                               "cannot infer generic parameter '%s'",
                               gp_name);
          ctx->error_count++;
        } else if (ta->impl->kind == TYPE_GENERIC_PACK &&
                   vec_get_size(ta->impl->generic_pack.expanded_types) == 0) {
          /* Pack parameter with empty expansion: not an error (empty pack is valid) */
        }
      }
    }

    /* Instantiate the function */
    if (type_args) {
      /* Check generic param constraints */
      _check_generic_param_constraints(ctx, generic_func_sym->function.generic_params,
          type_args, expr);

      /* Instantiate the function (takes ownership of type_args).
         Normal substitution handles pack indexing (Args[N]) correctly. */
      semantic_type_t inst_type = _instantiate_function(ctx, generic_func_sym,
          type_args, expr);
      if (inst_type->impl->kind != TYPE_ERROR) {
        callee_type = inst_type;
      }
    }

    allocator_free(ctx->allocator, &generic_param_names);
    allocator_free(ctx->allocator, &arg_types);
    if (explicit_type_args)
      allocator_free(ctx->allocator, &explicit_type_args);

    /* Arguments already checked above, verify against instantiated params */
    vec_t params = callee_type->impl->function.params;
    size_t param_count = params ? vec_get_size(params) : 0;
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

    /* For the 'cast' builtin, validate that the source type can be
       explicitly cast to the target (return) type. */
    if (call->callee->kind == CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION) {
      const char *gname = _checker_ident_str(
          ((cubec_expression_generic_instantiation_t)call->callee)->callee);
      if (gname && strcmp(gname, "cast") == 0 && arg_count >= 1 &&
          callee_type->impl->kind == TYPE_FUNCTION) {
        semantic_type_t return_type = callee_type->impl->function.return_type;
        node_t arg0 = (node_t)vec_get(call->arguments, 0);
        semantic_type_t arg_type = _check_expression(ctx, arg0);
        if (return_type->impl->kind != TYPE_ERROR && arg_type->impl->kind != TYPE_ERROR) {
          if (!semantic_type_can_explicit_cast(arg_type, return_type)) {
            diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                                 expr->location,
                                 "cannot cast '%s' to '%s'",
                                 arg_type->name ? arg_type->name : "<anonymous>",
                                 return_type->name ? return_type->name : "<anonymous>");
            ctx->error_count++;
          }
        }
      }
    }

    return callee_type->impl->function.return_type;
  }

  vec_t params = callee_type->impl->function.params;
  size_t param_count = params ? vec_get_size(params) : 0;
  bool is_variadic = callee_type->impl->function.is_variadic;

  /* Member call desugaring: a.method(args) → typeof(a)::method(&a, args)
     If callee is a member expression resolving to an instance method,
     the first param (self) is implicitly satisfied by &a (or p for pointers).
     Offset user arguments by 1 when matching against params. */
  bool is_member_call = false;
  if (call->callee->kind == CUBEC_NODE_EXPRESSION_MEMBER) {
    cubec_expression_member_t mem = (cubec_expression_member_t)call->callee;
    semantic_type_t host_type = _check_expression(ctx, mem->host);
    const char *fname = _checker_ident_str((node_t)mem->field);

    /* Determine the receiver type: dereference pointer for auto-deref */
    semantic_type_t receiver_type = host_type;
    if (host_type->impl->kind == TYPE_POINTER)
      receiver_type = host_type->impl->pointer.pointee;

    if (fname && receiver_type &&
        (receiver_type->impl->kind == TYPE_STRUCT ||
         receiver_type->impl->kind == TYPE_UNION ||
         receiver_type->impl->kind == TYPE_CUNION)) {
      /* Check if the member resolves to an instance method */
      if (receiver_type->instance_methods) {
        size_t mc = vec_get_size(receiver_type->instance_methods);
        for (size_t i = 0; i < mc; i++) {
          struct symbol *m = (struct symbol *)vec_get(receiver_type->instance_methods, i);
          if (m && m->name && strcmp(m->name, fname) == 0 && m->kind == SYMBOL_FUNCTION) {
            is_member_call = true;
            break;
          }
        }
      }
    }

    if (is_member_call) {
      /* The first param is self (*ReceiverType), automatically satisfied.
         User args start matching from param index 1. */
      if (param_count > 0 && !is_variadic && arg_count != param_count - 1) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                             "expected %zu arguments (excluding self), got %zu",
                             param_count - 1, arg_count);
        ctx->error_count++;
      }
      /* Check that first param is a pointer to the receiver type */
      if (param_count > 0) {
        semantic_type_t self_param = (semantic_type_t)vec_get(params, 0);
        semantic_type_t expected_self;
        if (host_type->impl->kind == TYPE_POINTER) {
          /* Pointer host: self param should match the pointer type directly */
          expected_self = host_type;
        } else {
          /* Object host: self param should be *typeof(host) */
          expected_self = semantic_type_create_pointer(ctx->allocator, host_type);
          vec_push(ctx->all_types, expected_self);
        }
        if (self_param->impl->kind != TYPE_POINTER ||
            !semantic_type_equals(self_param->impl->pointer.pointee,
                                  host_type->impl->kind == TYPE_POINTER
                                      ? host_type->impl->pointer.pointee
                                      : host_type)) {
          /* Soft warning: self param type mismatch — don't block compilation */
        }
      }
      /* Check user arguments against params[1..] */
      for (size_t i = 0; i < arg_count; i++) {
        node_t arg = (node_t)vec_get(call->arguments, i);
        semantic_type_t at = _check_expression(ctx, arg);
        size_t pidx = i + 1; /* offset by self param */
        if (pidx < param_count && at->impl->kind != TYPE_ERROR) {
          semantic_type_t pt = (semantic_type_t)vec_get(params, pidx);
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
  }

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

/* Check if a struct field is accessible from the current context.
   Currently all code is in the same module, so only pub matters for future
   cross-module access. When module system is implemented, this will check
   whether the accessing scope is in the same module as the struct definition. */
static bool _is_field_accessible(checker_t ctx,
                                  struct symbol *field_sym,
                                  semantic_type_t owner_type) {
  /* TODO(Phase 8): check cross-module access when module system is implemented.
     If field is not pub and accessing scope is in a different module, return false. */
  (void)ctx;
  (void)owner_type;
  return true;
}

static semantic_type_t _check_expr_member(checker_t ctx, node_t expr) {
  cubec_expression_member_t mem = (cubec_expression_member_t)expr;
  semantic_type_t host_type = _check_expression(ctx, mem->host);
  if (host_type->impl->kind == TYPE_ERROR) return ctx->error_type;

  const char *fname = _checker_ident_str((node_t)mem->field);
  if (!fname) return ctx->error_type;

  /* Determine the effective host type (strip qualifier wrapper, deref pointers)
     and whether const should propagate to field access results */
  semantic_type_t effective_host = host_type;
  bool host_is_const = semantic_type_is_const(host_type);

  /* Strip outer qualifier wrapper (e.g. const Struct → Struct) */
  if (host_type->impl->kind == TYPE_QUALIFIER) {
    effective_host = semantic_type_strip_qualifier(host_type);
  }

  if (_is_struct_like(effective_host)) {
    vec_t fields = _get_struct_fields(effective_host);
    size_t fcount = fields ? vec_get_size(fields) : 0;
    for (size_t i = 0; i < fcount; i++) {
      struct symbol *f = (struct symbol *)vec_get(fields, i);
      if (f && f->name && strcmp(f->name, fname) == 0) {
        if (!_is_field_accessible(ctx, f, effective_host)) {
          diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                               expr->location,
                               "field '%s' is private", fname);
          ctx->error_count++;
          return ctx->error_type;
        }
        semantic_type_t ft = f->field.type;
        if (host_is_const && !semantic_type_is_const(ft)) {
          semantic_type_t cft = semantic_type_create_qualifier(
              ctx->allocator, ft, true, false);
          type_hash_ensure(cft);
          vec_push(ctx->all_types, cft);
          return cft;
        }
        return ft;
      }
    }
    /* Methods don't need const propagation — they are function values */
    size_t mcount = vec_get_size(effective_host->instance_methods);
    for (size_t i = 0; i < mcount; i++) {
      struct symbol *m = (struct symbol *)vec_get(effective_host->instance_methods, i);
      if (m && m->name && strcmp(m->name, fname) == 0)
        return m->function.type;
    }
  }

  /* Pointer auto-dereference: ptr.field is equivalent to (*ptr).field */
  if (effective_host->impl->kind == TYPE_POINTER) {
    semantic_type_t pointee = effective_host->impl->pointer.pointee;
    /* Check if pointee is const (for *const T pointers) */
    bool pointee_is_const = semantic_type_is_const(pointee);
    semantic_type_t pointee_unq = semantic_type_strip_qualifier(pointee);

    if (_is_struct_like(pointee_unq)) {
      vec_t fields = _get_struct_fields(pointee_unq);
      size_t fcount = fields ? vec_get_size(fields) : 0;
      for (size_t i = 0; i < fcount; i++) {
        struct symbol *f = (struct symbol *)vec_get(fields, i);
        if (f && f->name && strcmp(f->name, fname) == 0) {
          if (!_is_field_accessible(ctx, f, pointee_unq)) {
            diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                                 expr->location,
                                 "field '%s' is private", fname);
            ctx->error_count++;
            return ctx->error_type;
          }
          semantic_type_t ft = f->field.type;
          if (pointee_is_const && !semantic_type_is_const(ft)) {
            semantic_type_t cft = semantic_type_create_qualifier(
                ctx->allocator, ft, true, false);
            type_hash_ensure(cft);
            vec_push(ctx->all_types, cft);
            return cft;
          }
          return ft;
        }
      }
      size_t mcount = vec_get_size(pointee_unq->instance_methods);
      for (size_t i = 0; i < mcount; i++) {
        struct symbol *m = (struct symbol *)vec_get(pointee_unq->instance_methods, i);
        if (m && m->name && strcmp(m->name, fname) == 0)
          return m->function.type;
      }
    }
  }

  if (effective_host->impl->kind == TYPE_INTERFACE) {
    vec_t methods = effective_host->impl->interface_type.methods;
    size_t mcount = methods ? vec_get_size(methods) : 0;
    for (size_t i = 0; i < mcount; i++) {
      struct symbol *m = (struct symbol *)vec_get(methods, i);
      if (m && m->name && strcmp(m->name, fname) == 0)
        return m->function.type;
    }
  }

  /* Enum member access: Color.Red → returns the enum type */
  if (effective_host->impl->kind == TYPE_ENUM) {
    vec_t items = effective_host->impl->enum_type.items;
    size_t icount = items ? vec_get_size(items) : 0;
    for (size_t i = 0; i < icount; i++) {
      struct symbol *item = (struct symbol *)vec_get(items, i);
      if (item && item->name && strcmp(item->name, fname) == 0)
        return effective_host;
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

  /* Enum member access: Color::Red → returns the enum type */
  if (host_type->impl->kind == TYPE_ENUM) {
    vec_t items = host_type->impl->enum_type.items;
    size_t icount = items ? vec_get_size(items) : 0;
    for (size_t i = 0; i < icount; i++) {
      struct symbol *item = (struct symbol *)vec_get(items, i);
      if (item && item->name && strcmp(item->name, fname) == 0)
        return host_type;
    }
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
  semantic_type_t host_type = _check_expression(ctx, pf->right);
  if (host_type->impl->kind == TYPE_ERROR) return ctx->error_type;

  /* Strip qualifier wrapper (e.g. const *T) to find the underlying pointer */
  semantic_type_t unqualified = semantic_type_strip_qualifier(host_type);
  if (unqualified->impl->kind != TYPE_POINTER) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "cannot dereference non-pointer type");
    ctx->error_count++;
    return ctx->error_type;
  }
  return unqualified->impl->pointer.pointee;
}

static semantic_type_t _check_expr_addr(checker_t ctx, node_t expr) {
  cubec_expression_postfix_unary_t pf =
      (cubec_expression_postfix_unary_t)expr;
  semantic_type_t host_type = _check_expression(ctx, pf->right);
  if (host_type->impl->kind == TYPE_ERROR) return ctx->error_type;

  if (!_is_lvalue(pf->right)) {
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
  semantic_type_t host_type = _check_expression(ctx, pf->right);
  if (host_type->impl->kind == TYPE_ERROR) return ctx->error_type;

  /* .? on pointer: dereference */
  if (host_type->impl->kind == TYPE_POINTER)
    return host_type->impl->pointer.pointee;

  /* .? on tagged union: check tag, return value type */
  semantic_type_t unq = semantic_type_strip_qualifier(host_type);
  vec_t fields = NULL;
  if (unq->impl->kind == TYPE_UNION)
    fields = unq->impl->struct_type.fields;
  else if (unq->impl->kind == TYPE_GENERIC_INSTANCE) {
    semantic_type_t base = unq->impl->generic_instance.generic_template;
    if (base && base->impl->kind == TYPE_UNION)
      fields = unq->impl->generic_instance.fields;
  }

  if (fields) {
    size_t fc = vec_get_size(fields);
    if (fc < 2) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                           ".? requires a union with at least 2 variants");
      ctx->error_count++;
      return ctx->error_type;
    }
    struct symbol *value_field = (struct symbol *)vec_get(fields, 0);
    return value_field->field.type;
  }

  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                       ".? requires a pointer or tagged union type");
  ctx->error_count++;
  return ctx->error_type;
}

static semantic_type_t _check_expr_assert(checker_t ctx, node_t expr) {
  cubec_expression_postfix_unary_t pf =
      (cubec_expression_postfix_unary_t)expr;
  semantic_type_t host_type = _check_expression(ctx, pf->right);
  if (host_type->impl->kind == TYPE_ERROR) return ctx->error_type;

  /* .! on pointer: assert non-null, return pointee */
  if (host_type->impl->kind == TYPE_POINTER)
    return host_type->impl->pointer.pointee;

  /* .! on tagged union: assert correct variant, return value type */
  semantic_type_t unq = semantic_type_strip_qualifier(host_type);
  vec_t fields = NULL;
  if (unq->impl->kind == TYPE_UNION)
    fields = unq->impl->struct_type.fields;
  else if (unq->impl->kind == TYPE_GENERIC_INSTANCE) {
    semantic_type_t base = unq->impl->generic_instance.generic_template;
    if (base && base->impl->kind == TYPE_UNION)
      fields = unq->impl->generic_instance.fields;
  }

  if (fields) {
    size_t fc = vec_get_size(fields);
    if (fc < 2) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                           ".! requires a union with at least 2 variants");
      ctx->error_count++;
      return ctx->error_type;
    }
    struct symbol *value_field = (struct symbol *)vec_get(fields, 0);
    return value_field->field.type;
  }

  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                       ".! requires a pointer or tagged union type");
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
  /* Try resolver_resolve_type for type expressions (sizeof(i32)),
   * fall back to _check_expression for value expressions (sizeof(x)).
   * Clear resolver error if it fails — _check_expression handles the fallback. */
  size_t err_before = ctx->error_count;
  semantic_type_t t = resolver_resolve_type(ctx, sz->expression);
  if (!t || t->impl->kind == TYPE_ERROR) {
    if (ctx->error_count > err_before) ctx->error_count = err_before;
    t = _check_expression(ctx, sz->expression);
  }
  if (!t || t->impl->kind == TYPE_ERROR) return ctx->error_type;
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
  size_t err_before = ctx->error_count;
  semantic_type_t t = resolver_resolve_type(ctx, al->expression);
  if (!t || t->impl->kind == TYPE_ERROR) {
    if (ctx->error_count > err_before) ctx->error_count = err_before;
    t = _check_expression(ctx, al->expression);
  }
  if (!t || t->impl->kind == TYPE_ERROR) return ctx->error_type;
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

  /* __slice__ magic method */
  if (ht->instance_methods) {
    size_t mc = vec_get_size(ht->instance_methods);
    for (size_t i = 0; i < mc; i++) {
      struct symbol *s = (struct symbol *)vec_get(ht->instance_methods, i);
      if (s && s->name && strcmp(s->name, "__slice__") == 0 &&
          s->kind == SYMBOL_FUNCTION && s->function.type) {
        return s->function.type->impl->function.return_type;
      }
    }
  }

  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                       "cannot slice type '%s'",
                       ht->name ? ht->name : "<anonymous>");
  ctx->error_count++;
  return ctx->error_type;
}

static semantic_type_t _check_expr_function(checker_t ctx, node_t expr) {
  cubec_expression_function_t fn = (cubec_expression_function_t)expr;

  func_check_info_t info;
  func_check_info_from_expression(&info, fn);

  semantic_type_t ret_type = info.return_type
      ? resolver_resolve_type(ctx, info.return_type) : ctx->builtin_void;
  vec_t param_types = _resolve_func_param_types(ctx, &info);

  /* Check function body using unified helper — includes return exhaustiveness */
  _check_func_body_and_returns(ctx, &info, ret_type, param_types,
                                ctx->current_scope);

  semantic_type_t ftype = semantic_type_create_function(
      ctx->allocator, ret_type, param_types, info.is_c_variadic);
  type_hash_ensure(ftype);
  vec_push(ctx->all_types, ftype);
  return ftype;
}

static semantic_type_t _check_expr_initialize_list(checker_t ctx, node_t expr) {
  cubec_expression_initialize_list_t il =
      (cubec_expression_initialize_list_t)expr;

  if (il->type) {
    /* Explicit type: .Type{...} */
    semantic_type_t t = resolver_resolve_type(ctx, il->type);
    if (t->impl->kind == TYPE_ERROR) return ctx->error_type;

    if (_is_struct_like(t) || t->impl->kind == TYPE_TUPLE) {
      vec_t fields = t->impl->kind == TYPE_TUPLE
          ? t->impl->tuple.fields
          : _get_struct_fields(t);
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

  /* Anonymous: infer type from content */
  if (il->is_field && il->items && vec_get_size(il->items) > 0) {
    /* Named fields → anonymous struct */
    semantic_type_t anon = semantic_type_create_named(ctx->allocator, NULL,
                                                       TYPE_STRUCT);
    vec_init_t vi = {.auto_dispose = true};
    anon->impl->struct_type.fields =
        (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

    size_t icount = vec_get_size(il->items);
    for (size_t i = 0; i < icount; i++) {
      node_t item = (node_t)vec_get(il->items, i);
      if (item->kind != CUBEC_NODE_EXPRESSION_INITIALIZE_FIELD) continue;
      cubec_expression_initialize_field_t f =
          (cubec_expression_initialize_field_t)item;
      const char *fname = _checker_ident_str((node_t)f->field);
      semantic_type_t ftype = f->value
          ? _check_expression(ctx, f->value)
          : ctx->error_type;
      struct symbol *fsym = symbol_create(ctx->allocator, fname, SYMBOL_FIELD,
                                          item->location);
      fsym->field.type = ftype;
      fsym->field.index = i;
      fsym->field.is_pub = true;
      vec_push(anon->impl->struct_type.fields, fsym);
    }
    type_layout_compute(anon, 8);
    type_hash_ensure(anon);
    vec_push(ctx->all_types, anon);
    return anon;
  } else if (il->items && vec_get_size(il->items) > 0) {
    /* Positional fields → tuple */
    size_t icount = vec_get_size(il->items);
    vec_init_t vi = {.auto_dispose = false};
    vec_t elem_types = (vec_t)allocator_create(ctx->allocator, &g_vec_type,
                                                &vi);
    for (size_t i = 0; i < icount; i++) {
      node_t item = (node_t)vec_get(il->items, i);
      semantic_type_t et = _check_expression(ctx, item);
      vec_push(elem_types, et);
    }
    semantic_type_t tuple = semantic_type_create_tuple(ctx->allocator,
                                                        elem_types);
    type_layout_compute(tuple, 8);
    type_hash_ensure(tuple);
    vec_push(ctx->all_types, tuple);
    return tuple;
  } else {
    /* Empty .{} → empty struct */
    semantic_type_t empty = semantic_type_create_named(ctx->allocator, NULL,
                                                        TYPE_STRUCT);
    vec_init_t vi = {.auto_dispose = true};
    empty->impl->struct_type.fields =
        (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
    type_layout_compute(empty, 8);
    type_hash_ensure(empty);
    vec_push(ctx->all_types, empty);
    return empty;
  }
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
  case CUBEC_NODE_LITERAL_UNDEFINED:    return _check_expr_literal_undefined(ctx, expr);
  case CUBEC_NODE_EXPRESSION_BINARY:     return _check_expr_binary(ctx, expr);
  case CUBEC_NODE_EXPRESSION_ASSIGNMENT: return _check_expr_assignment(ctx, expr);
  case CUBEC_NODE_EXPRESSION_CALL:       return _check_expr_call(ctx, expr);
  case CUBEC_NODE_EXPRESSION_MEMBER:     return _check_expr_member(ctx, expr);
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS: return _check_expr_namespace_access(ctx, expr);
  case CUBEC_NODE_EXPRESSION_DEREF:      return _check_expr_deref(ctx, expr);
  case CUBEC_NODE_EXPRESSION_ADDR:       return _check_expr_addr(ctx, expr);
  case CUBEC_NODE_EXPRESSION_TRY:        return _check_expr_try(ctx, expr);
  case CUBEC_NODE_EXPRESSION_ASSERT:     return _check_expr_assert(ctx, expr);
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
  case CUBEC_NODE_EXPRESSION_TYPE_TUPLE:
  case CUBEC_NODE_EXPRESSION_TYPE_ENUM:
  case CUBEC_NODE_EXPRESSION_TYPE_UNION:
  case CUBEC_NODE_EXPRESSION_TYPE_INTERFACE:
  case CUBEC_NODE_EXPRESSION_TYPE_FUNCTION:
  case CUBEC_NODE_EXPRESSION_WILDCARD:
    return resolver_resolve_type(ctx, expr);
  default: return ctx->error_type;
  }
}

semantic_type_t checker_check_expression(checker_t ctx, node_t expr) {
  return _check_expression(ctx, expr);
}
