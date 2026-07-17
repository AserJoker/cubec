#include "engine/checker.h"
#include "engine/checker_evaluate.h"
#include "engine/checker_type_util.h"
#include "engine/checker_check_expr.h"
#include "engine/checker_check_stmt.h"
#include "engine/comptime_eval.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
#include "core/allocator.h"
#include "cubec/statement_test.h"
#include "core/string.h"
#include "core/vec.h"
#include "cubec/node.h"
#include "cubec/program.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_enum.h"
#include "cubec/statement_union.h"
#include "cubec/statement_cunion.h"
#include "cubec/statement_interface.h"
#include "cubec/statement_function.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/statement_import.h"
#include "cubec/statement_comptime.h"
#include "cubec/struct_field.h"
#include "cubec/enum_item.h"
#include "cubec/union_field.h"
#include "cubec/function_argument.h"
#include "cubec/interface_method.h"
#include "cubec/generic_param.h"
#include "cubec/decorator.h"
#include "cubec/declaration_variable.h"
#include <string.h>

/* ===== Pass 2: Declaration Evaluation ===== */

semantic_type_t _check_expression(checker_t ctx, node_t expr);

static void _evaluate_member_method(checker_t ctx, semantic_type_t t,
                                     cubec_statement_function_t mfn) {
  const char *mname = _checker_ident_str(mfn->name);
  struct symbol *msym = symbol_create(ctx->allocator, mname,
                                      SYMBOL_FUNCTION, mfn->super.location);
  semantic_type_t ret_type = mfn->return_type
      ? resolver_resolve_type(ctx, mfn->return_type)
      : ctx->builtin_void;
  vec_init_t pvi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &pvi);
  if (mfn->arguments) {
    size_t acount = vec_get_size(mfn->arguments);
    for (size_t j = 0; j < acount; j++) {
      node_t arg = (node_t)vec_get(mfn->arguments, j);
      if (arg->kind == CUBEC_NODE_FUNCTION_ARGUMENT) {
        cubec_function_argument_t farg = (cubec_function_argument_t)arg;
        if (farg->type) {
          semantic_type_t pt = resolver_resolve_type(ctx, farg->type);
          vec_push(params, pt);
        }
      }
    }
  }
  semantic_type_t mtype = semantic_type_create_function(
      ctx->allocator, ret_type, params, mfn->is_c_variadic);
  type_hash_ensure(mtype);
  vec_push(ctx->all_types, mtype);
  msym->function.type = mtype;
  msym->function.is_comptime = mfn->is_comptime;
  msym->function.ast_node = (node_t)mfn;
  msym->state = SYMBOL_NAME_KNOWN; /* body checked in Pass 3 */
  vec_push(t->instance_methods, msym);
}

static void _evaluate_member_declaration(checker_t ctx, semantic_type_t t,
                                         cubec_statement_declaration_t sdecl) {
  cubec_declaration_variable_t vdecl =
      (cubec_declaration_variable_t)sdecl->declarator;
  if (!vdecl) return;
  const char *vname = _checker_ident_str(vdecl->identifier);
  struct symbol *vsym = symbol_create(ctx->allocator, vname,
                                      SYMBOL_VARIABLE, sdecl->super.location);
  if (vdecl->type)
    vsym->variable.type = resolver_resolve_type(ctx, vdecl->type);
  vsym->variable.is_comptime = sdecl->is_comptime;
  vsym->variable.is_mutable = true;
  vsym->state = SYMBOL_NAME_KNOWN; /* initializer in Pass 3 */
  vec_push(t->static_fields, vsym);
}

static void _evaluate_struct_union_members(checker_t ctx, semantic_type_t t,
                                           vec_t members) {
  if (!members) return;
  size_t mcount = vec_get_size(members);
  for (size_t i = 0; i < mcount; i++) {
    node_t member = (node_t)vec_get(members, i);
    if (!member) continue;
    if (member->kind == CUBEC_NODE_STATEMENT_FUNCTION)
      _evaluate_member_method(ctx, t, (cubec_statement_function_t)member);
    else if (member->kind == CUBEC_NODE_STATEMENT_DECLARATION)
      _evaluate_member_declaration(ctx, t, (cubec_statement_declaration_t)member);
  }
}

static void _evaluate_struct(checker_t ctx, cubec_statement_struct_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_TYPE || !sym->type.type) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  semantic_type_t t = sym->type.type;

  /* Generic struct: mark evaluated, skip field resolution */
  if (node->generic_params) {
    sym->state = SYMBOL_EVALUATED;
    return;
  }

  /* Create fields vec */
  vec_init_t vi = {.auto_dispose = true};
  t->impl->struct_type.fields =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  /* Resolve struct fields */
  if (node->members) {
    size_t mcount = vec_get_size(node->members);
    for (size_t i = 0; i < mcount; i++) {
      node_t member = (node_t)vec_get(node->members, i);
      if (!member) continue;

      if (member->kind == CUBEC_NODE_STRUCT_FIELD) {
        cubec_struct_field_t field = (cubec_struct_field_t)member;
        const char *fname = _checker_ident_str(field->name);
        struct symbol *fsym = symbol_create(ctx->allocator, fname,
                                            SYMBOL_FIELD, field->super.location);
        if (field->type)
          fsym->field.type = resolver_resolve_type(ctx, field->type);
        fsym->field.index = i;
        fsym->field.is_pub = field->is_pub;
        vec_push(t->impl->struct_type.fields, fsym);
      }
    }
  }

  /* Resolve methods and static fields */
  _evaluate_struct_union_members(ctx, t, node->members);

  type_layout_compute(t, 8);
  type_hash_ensure(t);
  sym->state = SYMBOL_EVALUATED;
}

static void _evaluate_enum_items(checker_t ctx, semantic_type_t t, vec_t items) {
  if (!items) return;
  size_t count = vec_get_size(items);
  long long auto_val = 0;
  for (size_t i = 0; i < count; i++) {
    node_t item_node = (node_t)vec_get(items, i);
    if (!item_node || item_node->kind != CUBEC_NODE_ENUM_ITEM) continue;

    cubec_enum_item_t item = (cubec_enum_item_t)item_node;
    const char *iname = _checker_ident_str(item->name);
    struct symbol *isym = symbol_create(ctx->allocator, iname,
                                        SYMBOL_ENUM_ITEM, item->super.location);
    isym->enum_item.owning_type = t;
    /* Evaluate explicit value if present, otherwise auto-increment */
    if (item->value) {
      if (item->value->kind == CUBEC_NODE_LITERAL_NUMERIC) {
        cubec_literal_numeric_t num = (cubec_literal_numeric_t)item->value;
        const char *numstr = string_get(num->value);
        isym->enum_item.value = numstr ? atoll(numstr) : auto_val;
      } else {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             item->value->location,
                             "enum value must be a compile-time integer literal");
        ctx->error_count++;
        isym->enum_item.value = auto_val;
      }
      auto_val = isym->enum_item.value + 1;
    } else {
      isym->enum_item.value = auto_val++;
    }
    vec_push(t->impl->enum_type.items, isym);
  }
}

static void _evaluate_enum(checker_t ctx, cubec_statement_enum_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_TYPE || !sym->type.type) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  semantic_type_t t = sym->type.type;

  vec_init_t vi = {.auto_dispose = true};
  t->impl->enum_type.items =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  t->impl->enum_type.backing_type = ctx->builtin_i32;

  _evaluate_enum_items(ctx, t, node->items);

  type_layout_compute(t, 8);
  type_hash_ensure(t);
  sym->state = SYMBOL_EVALUATED;
}

static void _evaluate_union(checker_t ctx, cubec_statement_union_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_TYPE || !sym->type.type) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  semantic_type_t t = sym->type.type;

  /* Generic union: mark evaluated, skip field resolution */
  if (node->generic_params) {
    sym->state = SYMBOL_EVALUATED;
    return;
  }

  vec_init_t vi = {.auto_dispose = true};
  t->impl->struct_type.fields =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  /* Resolve union fields */
  if (node->members) {
    size_t mcount = vec_get_size(node->members);
    for (size_t i = 0; i < mcount; i++) {
      node_t member = (node_t)vec_get(node->members, i);
      if (!member) continue;

      if (member->kind == CUBEC_NODE_UNION_FIELD) {
        cubec_union_field_t field = (cubec_union_field_t)member;
        const char *fname = _checker_ident_str(field->name);
        struct symbol *fsym = symbol_create(ctx->allocator, fname,
                                            SYMBOL_FIELD, field->super.location);
        if (field->type)
          fsym->field.type = resolver_resolve_type(ctx, field->type);
        fsym->field.index = i;
        vec_push(t->impl->struct_type.fields, fsym);
      }
    }
  }

  /* Resolve methods and static fields */
  _evaluate_struct_union_members(ctx, t, node->members);

  type_layout_compute(t, 8);
  type_hash_ensure(t);
  sym->state = SYMBOL_EVALUATED;
}

static void _evaluate_cunion(checker_t ctx, cubec_statement_cunion_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_TYPE || !sym->type.type) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  semantic_type_t t = sym->type.type;

  vec_init_t vi = {.auto_dispose = true};
  t->impl->struct_type.fields =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  if (node->fields) {
    size_t count = vec_get_size(node->fields);
    for (size_t i = 0; i < count; i++) {
      node_t field_node = (node_t)vec_get(node->fields, i);
      if (!field_node || field_node->kind != CUBEC_NODE_STRUCT_FIELD) continue;

      cubec_struct_field_t field = (cubec_struct_field_t)field_node;
      const char *fname = _checker_ident_str(field->name);
      struct symbol *fsym = symbol_create(ctx->allocator, fname,
                                          SYMBOL_FIELD, field->super.location);
      if (field->type)
        fsym->field.type = resolver_resolve_type(ctx, field->type);
      fsym->field.index = i;
      fsym->field.is_pub = field->is_pub;
      vec_push(t->impl->struct_type.fields, fsym);
    }
  }

  type_layout_compute(t, 8);
  type_hash_ensure(t);
  sym->state = SYMBOL_EVALUATED;
}

static void _evaluate_interface_method(checker_t ctx, semantic_type_t t,
                                         cubec_interface_method_t method) {
  const char *mname = _checker_ident_str(method->name);
  struct symbol *msym = symbol_create(ctx->allocator, mname,
                                      SYMBOL_FUNCTION, method->super.location);

  /* Resolve return type */
  semantic_type_t ret_type = method->return_type
      ? resolver_resolve_type(ctx, method->return_type)
      : ctx->builtin_void;

  /* Resolve parameters */
  vec_init_t pvi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &pvi);
  if (method->arguments) {
    size_t acount = vec_get_size(method->arguments);
    for (size_t j = 0; j < acount; j++) {
      node_t arg = (node_t)vec_get(method->arguments, j);
      if (arg->kind == CUBEC_NODE_FUNCTION_ARGUMENT) {
        cubec_function_argument_t farg = (cubec_function_argument_t)arg;
        if (farg->type) {
          semantic_type_t pt = resolver_resolve_type(ctx, farg->type);
          vec_push(params, pt);
        }
      }
    }
  }

  semantic_type_t mtype = semantic_type_create_function(
      ctx->allocator, ret_type, params, false);
  type_hash_ensure(mtype);
  vec_push(ctx->all_types, mtype);
  msym->function.type = mtype;
  msym->state = SYMBOL_EVALUATED;
  vec_push(t->impl->interface_type.methods, msym);
}

static void _evaluate_associated_type(checker_t ctx, semantic_type_t t,
                                       cubec_statement_declaration_type_t tdecl) {
  const char *tname = _checker_ident_str(tdecl->name);
  struct symbol *tsym = symbol_create(ctx->allocator, tname,
                                      SYMBOL_TYPE, tdecl->super.location);
  if (tdecl->type_value)
    tsym->type.type = resolver_resolve_type(ctx, tdecl->type_value);
  tsym->state = SYMBOL_NAME_KNOWN;
  vec_push(t->associated_types, tsym);
}

static void _evaluate_interface(checker_t ctx,
                                cubec_statement_interface_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_TYPE || !sym->type.type) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  semantic_type_t t = sym->type.type;

  /* Generic interface: mark evaluated, skip method resolution */
  if (node->generic_params) {
    sym->state = SYMBOL_EVALUATED;
    return;
  }

  vec_init_t vi = {.auto_dispose = true};
  t->impl->interface_type.methods =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  if (node->members) {
    size_t count = vec_get_size(node->members);
    for (size_t i = 0; i < count; i++) {
      node_t member = (node_t)vec_get(node->members, i);
      if (!member) continue;
      if (member->kind == CUBEC_NODE_INTERFACE_METHOD)
        _evaluate_interface_method(ctx, t, (cubec_interface_method_t)member);
      else if (member->kind == CUBEC_NODE_STATEMENT_DECLARATION_TYPE)
        _evaluate_associated_type(ctx, t, (cubec_statement_declaration_type_t)member);
    }
  }

  type_layout_compute(t, 8);
  type_hash_ensure(t);
  sym->state = SYMBOL_EVALUATED;
}

static void _evaluate_function(checker_t ctx,
                               cubec_statement_function_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_FUNCTION) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  /* Builtin function: resolve type signature only, no body, no comptime binding */
  if (node->is_builtin) {
    semantic_type_t ret_type = node->return_type
        ? resolver_resolve_type(ctx, node->return_type)
        : ctx->builtin_void;

    vec_init_t vi = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
    if (node->arguments) {
      size_t count = vec_get_size(node->arguments);
      for (size_t i = 0; i < count; i++) {
        node_t arg = (node_t)vec_get(node->arguments, i);
        if (arg->kind == CUBEC_NODE_FUNCTION_ARGUMENT) {
          cubec_function_argument_t farg = (cubec_function_argument_t)arg;
          if (farg->type) {
            semantic_type_t pt = resolver_resolve_type(ctx, farg->type);
            vec_push(params, pt);
          }
        }
      }
    }

    semantic_type_t ftype = semantic_type_create_function(
        ctx->allocator, ret_type, params, node->is_c_variadic);
    type_hash_ensure(ftype);
    vec_push(ctx->all_types, ftype);

    sym->function.type = ftype;
    sym->function.is_comptime = false;
    sym->function.ast_node = NULL;
    sym->state = SYMBOL_EVALUATED;
    return;
  }

  /* Generic function: mark evaluated, skip signature resolution */
  if (node->generic_params) {
    sym->state = SYMBOL_EVALUATED;
    return;
  }

  /* Resolve return type */
  semantic_type_t ret_type = node->return_type
      ? resolver_resolve_type(ctx, node->return_type)
      : ctx->builtin_void;

  /* Resolve parameter types */
  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  if (node->arguments) {
    size_t count = vec_get_size(node->arguments);
    for (size_t i = 0; i < count; i++) {
      node_t arg = (node_t)vec_get(node->arguments, i);
      if (arg->kind == CUBEC_NODE_FUNCTION_ARGUMENT) {
        cubec_function_argument_t farg = (cubec_function_argument_t)arg;
        if (farg->type) {
          semantic_type_t pt = resolver_resolve_type(ctx, farg->type);
          vec_push(params, pt);
        }
      }
    }
  }

  semantic_type_t ftype = semantic_type_create_function(
      ctx->allocator, ret_type, params, node->is_c_variadic);
  type_hash_ensure(ftype);
  vec_push(ctx->all_types, ftype);

  sym->function.type = ftype;
  sym->function.is_comptime = node->is_comptime;
  sym->function.ast_node = (node_t)node;
  /* Body NOT checked in Pass 2 — deferred to Pass 3 */
  sym->state = SYMBOL_EVALUATED;

  /* Bind function in comptime env so it can be called at compile time */
  if (node->body) {
    vec_t param_names = NULL;
    if (node->arguments) {
      vec_init_t pvi = {.auto_dispose = false};
      param_names = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &pvi);
      size_t acount = vec_get_size(node->arguments);
      for (size_t i = 0; i < acount; i++) {
        node_t arg = (node_t)vec_get(node->arguments, i);
        if (arg->kind == CUBEC_NODE_FUNCTION_ARGUMENT) {
          cubec_function_argument_t farg = (cubec_function_argument_t)arg;
          const char *pname = _checker_ident_str(farg->identifier);
          if (pname) vec_push(param_names, (void *)pname);
        }
      }
    }
    comptime_value_t fn_val = comptime_value_create_function(
        ctx->allocator,
        ctx->comptime_eval->global_env,
        node->body, param_names, ftype);
    comptime_env_bind_value(ctx->comptime_eval->global_env,
                            ctx->comptime_eval->valloc, name, fn_val);
  }
}

static void _check_var_type_completeness(checker_t ctx, node_t loc_node,
                                           semantic_type_t var_type,
                                           const char *name) {
  if (var_type && var_type->impl->kind == TYPE_VOID) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, loc_node->location,
                         "variable '%s' has incomplete type 'void'", name);
    ctx->error_count++;
  } else if (var_type && var_type->is_incomplete &&
             var_type->impl->kind != TYPE_ERROR) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, loc_node->location,
                         "variable '%s' has incomplete type '%s'",
                         name, var_type->name ? var_type->name : "<anonymous>");
    ctx->error_count++;
  }
}

static void _evaluate_variable(checker_t ctx,
                               cubec_statement_declaration_t node) {
  cubec_declaration_variable_t decl =
      (cubec_declaration_variable_t)node->declarator;
  if (!decl) return;

  const char *name = _checker_ident_str(decl->identifier);
  if (!name) return;

  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_VARIABLE) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  semantic_type_t var_type = NULL;

  /* Explicit type annotation */
  if (decl->type)
    var_type = resolver_resolve_type(ctx, decl->type);

  /* Type inference from initializer expression */
  if (!var_type && decl->expression)
    var_type = _check_expression(ctx, decl->expression);

  /* extern/builtin require explicit type */
  if ((node->is_extern || node->is_builtin) && !decl->type) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "extern/builtin variable '%s' requires type annotation",
                         name);
    ctx->error_count++;
    var_type = ctx->error_type;
  }

  if (!var_type) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "cannot infer type for variable '%s'", name);
    ctx->error_count++;
    var_type = ctx->error_type;
  }

  _check_var_type_completeness(ctx, &node->super, var_type, name);

  sym->variable.type = var_type;
  sym->variable.is_comptime = node->is_comptime;
  sym->variable.is_mutable = true;
  sym->state = SYMBOL_EVALUATED;

  if (node->is_comptime && node->declarator &&
      node->declarator->kind == CUBEC_NODE_DECLARATION_VARIABLE) {
    cubec_declaration_variable_t dv =
        (cubec_declaration_variable_t)node->declarator;
    if (dv->expression) {
      comptime_value_t val =
          comptime_eval_expr(ctx->comptime_eval, ctx, dv->expression);
      if (val && val->kind != COMPTIME_VALUE_ERROR) {
        comptime_env_bind_value(ctx->comptime_eval->current_env
                              ? ctx->comptime_eval->current_env
                              : ctx->comptime_eval->global_env,
                          ctx->comptime_eval->valloc,
                          name, comptime_value_clone(ctx->allocator, val));
      }
    }
  }
}

static void _evaluate_type_alias(checker_t ctx,
                                 cubec_statement_declaration_type_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_TYPE) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  if (node->type_value) {
    semantic_type_t resolved = resolver_resolve_type(ctx, node->type_value);
    sym->type.type = resolved;
  } else if (!node->is_builtin) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "type alias '%s' requires a type expression", name);
    ctx->error_count++;
  }

  /* Generic type alias: still mark evaluated (template) */
  sym->state = SYMBOL_EVALUATED;
}

static void _evaluate_import(checker_t ctx,
                             cubec_statement_import_t node) {
  const char *name = _checker_ident_str(node->module_name);
  if (!name) return;

  const char *effective_name = node->alias ? _checker_ident_str(node->alias) : name;
  struct symbol *sym = scope_lookup_local(ctx->global_scope, effective_name);
  if (!sym || sym->kind != SYMBOL_MODULE) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  /* TODO: module resolution — load and check imported module */

  sym->state = SYMBOL_EVALUATED;
}

static void _evaluate_comptime_block(checker_t ctx,
                                     cubec_statement_comptime_block_t node) {
  if (!ctx->comptime_eval) return;
  comptime_signal_t sig =
      comptime_eval_exec_block(ctx->comptime_eval, ctx, node->body);
  if (sig.kind == COMPTIME_SIGNAL_ERROR) ctx->error_count++;
}

static void _evaluate_comptime_if(checker_t ctx,
                                  cubec_statement_comptime_if_t node) {
  if (!ctx->comptime_eval) return;
  comptime_signal_t sig =
      comptime_eval_exec_comptime_if(ctx->comptime_eval, ctx, (node_t)node);
  if (sig.kind == COMPTIME_SIGNAL_ERROR) ctx->error_count++;
}

static void _evaluate_comptime_for(checker_t ctx,
                                   cubec_statement_comptime_for_t node) {
  if (!ctx->comptime_eval) return;
  comptime_signal_t sig =
      comptime_eval_exec_comptime_for(ctx->comptime_eval, ctx, (node_t)node);
  if (sig.kind == COMPTIME_SIGNAL_ERROR) ctx->error_count++;
}

static void _evaluate_test(checker_t ctx,
                           cubec_statement_test_t node) {
  if (!ctx->comptime_eval) return;

  /* Check the test body for type errors before evaluating */
  int errors_before_check = ctx->error_count;
  _check_statement(ctx, node->body, NULL);

  ctx->test_count++;

  /* If checker found errors, skip eval and mark test as failed */
  if (ctx->error_count > errors_before_check) {
    ctx->test_fail_count++;
    return;
  }

  int errors_before_eval = ctx->error_count;
  comptime_signal_t sig =
      comptime_eval_exec_block(ctx->comptime_eval, ctx, node->body);
  if (sig.kind == COMPTIME_SIGNAL_ERROR || ctx->error_count > errors_before_eval) {
    ctx->test_fail_count++;
  }
}

void checker_evaluate_declarations(checker_t ctx, node_t program) {
  cubec_program_node_t prog = (cubec_program_node_t)program;
  if (!prog || !prog->statements) return;

  size_t count = vec_get_size(prog->statements);
  for (size_t i = 0; i < count; i++) {
    node_t stmt = (node_t)vec_get(prog->statements, i);
    if (!stmt) continue;
    switch (stmt->kind) {
    case CUBEC_NODE_STATEMENT_STRUCT:          _evaluate_struct(ctx, (cubec_statement_struct_t)stmt); break;
    case CUBEC_NODE_STATEMENT_ENUM:            _evaluate_enum(ctx, (cubec_statement_enum_t)stmt); break;
    case CUBEC_NODE_STATEMENT_UNION:           _evaluate_union(ctx, (cubec_statement_union_t)stmt); break;
    case CUBEC_NODE_STATEMENT_CUNION:          _evaluate_cunion(ctx, (cubec_statement_cunion_t)stmt); break;
    case CUBEC_NODE_STATEMENT_INTERFACE:       _evaluate_interface(ctx, (cubec_statement_interface_t)stmt); break;
    case CUBEC_NODE_STATEMENT_FUNCTION:        _evaluate_function(ctx, (cubec_statement_function_t)stmt); break;
    case CUBEC_NODE_STATEMENT_DECLARATION:     _evaluate_variable(ctx, (cubec_statement_declaration_t)stmt); break;
    case CUBEC_NODE_STATEMENT_DECLARATION_TYPE: _evaluate_type_alias(ctx, (cubec_statement_declaration_type_t)stmt); break;
    case CUBEC_NODE_STATEMENT_IMPORT:          _evaluate_import(ctx, (cubec_statement_import_t)stmt); break;
    case CUBEC_NODE_STATEMENT_COMPTIME_BLOCK:  _evaluate_comptime_block(ctx, (cubec_statement_comptime_block_t)stmt); break;
    case CUBEC_NODE_STATEMENT_COMPTIME_IF:     _evaluate_comptime_if(ctx, (cubec_statement_comptime_if_t)stmt); break;
    case CUBEC_NODE_STATEMENT_COMPTIME_FOR:    _evaluate_comptime_for(ctx, (cubec_statement_comptime_for_t)stmt); break;
    case CUBEC_NODE_STATEMENT_TEST:            _evaluate_test(ctx, (cubec_statement_test_t)stmt); break;
    default: break;
    }
  }
}
