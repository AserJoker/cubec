#include "engine/checker.h"
#include "engine/checker_decorator.h"
#include "engine/checker_evaluate.h"
#include "engine/checker_collect.h"
#include "engine/checker_func_util.h"
#include "engine/checker_type_util.h"
#include "engine/checker_check_expr.h"
#include "engine/checker_check_stmt.h"
#include "engine/comptime_eval.h"
#include "engine/comptime_eval_internal.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
#include "engine/module.h"
#include "engine/manifest.h"
#include "core/allocator.h"
#include "cubec/token.h"
#include "cubec/statement_test.h"
#include "cubec/statement_block.h"
#include "core/string.h"
#include "core/vec.h"
#include "cubec/node.h"
#include "cubec/program.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_string.h"
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
#include "cubec/statement_export_from.h"
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

/* _register_generic_params moved to checker_func_util.c as checker_register_generic_params */

static void _evaluate_member_method(checker_t ctx, semantic_type_t t,
                                     cubec_statement_function_t mfn,
                                     size_t type_gp_count) {
  func_check_info_t info;
  func_check_info_from_statement(&info, mfn);
  _process_function(ctx, &info, &(func_context_t){
      .symbol_scope = ctx->global_scope,
      .defer_body = true,
      .is_method = true,
      .host_type = t,
      .use_child_scope = true,
      .pre_existing_sym = NULL,
      .symbol_state = SYMBOL_NAME_KNOWN
  });
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
  vsym->variable.is_mutable = !semantic_type_is_const(vsym->variable.type);
  vsym->state = SYMBOL_NAME_KNOWN; /* initializer in Pass 3 */
  vec_push(t->static_fields, vsym);
}

void checker_evaluate_struct_union_members(checker_t ctx, semantic_type_t t,
                                           vec_t members, size_t type_gp_count) {
  if (!members) return;
  if (!t->instance_methods) return;
  size_t mcount = vec_get_size(members);
  for (size_t i = 0; i < mcount; i++) {
    node_t member = (node_t)vec_get(members, i);
    if (!member) continue;
    if (member->kind == CUBEC_NODE_STATEMENT_FUNCTION)
      _evaluate_member_method(ctx, t, (cubec_statement_function_t)member,
                              type_gp_count);
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

  /* Generic struct: register generic params and store template */
  if (node->generic_params) {
    checker_register_generic_params(ctx, node->generic_params);
    sym->type.generic_params = node->generic_params;
  }

  /* Resolve fields and methods */
  _resolve_struct_fields(ctx, t, node->members);
  {
    size_t type_gp_count = node->generic_params ? vec_get_size(node->generic_params) : 0;
    checker_evaluate_struct_union_members(ctx, t, node->members, type_gp_count);
  }

  /* Skip layout for generic — sizes depend on concrete type args */
  if (!node->generic_params) type_layout_compute(t, 8);
  type_hash_ensure(t);
  sym->state = SYMBOL_EVALUATED;

  /* Verify implement clauses (skip constraint check for generic — verified at instantiation) */
  if (node->implements) {
    vec_t impl_vec = (vec_t)allocator_create(
        ctx->allocator, &g_vec_type, &(vec_init_t){false});
    size_t icount = vec_get_size(node->implements);
    for (size_t i = 0; i < icount; i++) {
      node_t iface_expr = (node_t)vec_get(node->implements, i);
      semantic_type_t iface_type = resolver_resolve_type(ctx, iface_expr);
      if (!iface_type || iface_type->impl->kind == TYPE_ERROR) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
            iface_expr->location,
            "cannot resolve interface type in implement clause");
        ctx->error_count++;
        continue;
      }
      if (iface_type->impl->kind != TYPE_INTERFACE &&
          !(iface_type->impl->kind == TYPE_GENERIC_INSTANCE &&
            iface_type->impl->generic_instance.generic_template->impl->kind
                == TYPE_INTERFACE)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
            iface_expr->location,
            "implement clause requires an interface type, got '%s'",
            iface_type->name ? iface_type->name : "<anonymous>");
        ctx->error_count++;
        continue;
      }
      /* Skip constraint check for generic — verified at instantiation time */
      if (!node->generic_params) {
        _check_constraint(ctx, t, iface_type, (node_t)node);
      }
      vec_push(impl_vec, iface_type);
    }
    t->implements = impl_vec;
  }

  /* Evaluate decorators (skip for generic — evaluated at instantiation) */
  if (node->decorators && !node->generic_params)
    checker_evaluate_decorators(ctx, node->decorators, DECORATOR_TARGET_TYPE,
                                name, (node_t)node);
}

/* _evaluate_enum_items moved to checker_type_util.c as _resolve_enum_items */

static void _evaluate_enum(checker_t ctx, cubec_statement_enum_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_TYPE || !sym->type.type) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  semantic_type_t t = sym->type.type;

  t->impl->enum_type.backing_type = ctx->builtin_i32;

  _resolve_enum_items(ctx, t, node->items);

  type_layout_compute(t, 8);
  type_hash_ensure(t);
  sym->state = SYMBOL_EVALUATED;

  if (node->decorators)
    checker_evaluate_decorators(ctx, node->decorators, DECORATOR_TARGET_TYPE,
                                name, (node_t)node);
}

static void _evaluate_union(checker_t ctx, cubec_statement_union_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_TYPE || !sym->type.type) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  semantic_type_t t = sym->type.type;

  /* Generic union: register generic params and store template */
  if (node->generic_params) {
    checker_register_generic_params(ctx, node->generic_params);
    sym->type.generic_params = node->generic_params;
  }

  /* Resolve fields and methods */
  _resolve_union_fields(ctx, t, node->members);
  {
    size_t type_gp_count = node->generic_params ? vec_get_size(node->generic_params) : 0;
    checker_evaluate_struct_union_members(ctx, t, node->members, type_gp_count);
  }

  /* Skip layout for generic — sizes depend on concrete type args */
  if (!node->generic_params) type_layout_compute(t, 8);
  type_hash_ensure(t);
  sym->state = SYMBOL_EVALUATED;

  /* Verify implement clauses (skip constraint check for generic — verified at instantiation) */
  if (node->implements) {
    vec_t impl_vec = (vec_t)allocator_create(
        ctx->allocator, &g_vec_type, &(vec_init_t){false});
    size_t icount = vec_get_size(node->implements);
    for (size_t i = 0; i < icount; i++) {
      node_t iface_expr = (node_t)vec_get(node->implements, i);
      semantic_type_t iface_type = resolver_resolve_type(ctx, iface_expr);
      if (!iface_type || iface_type->impl->kind == TYPE_ERROR) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
            iface_expr->location,
            "cannot resolve interface type in implement clause");
        ctx->error_count++;
        continue;
      }
      if (iface_type->impl->kind != TYPE_INTERFACE &&
          !(iface_type->impl->kind == TYPE_GENERIC_INSTANCE &&
            iface_type->impl->generic_instance.generic_template->impl->kind
                == TYPE_INTERFACE)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
            iface_expr->location,
            "implement clause requires an interface type, got '%s'",
            iface_type->name ? iface_type->name : "<anonymous>");
        ctx->error_count++;
        continue;
      }
      /* Skip constraint check for generic — verified at instantiation time */
      if (!node->generic_params) {
        _check_constraint(ctx, t, iface_type, (node_t)node);
      }
      vec_push(impl_vec, iface_type);
    }
    t->implements = impl_vec;
  }

  /* Evaluate decorators (skip for generic — evaluated at instantiation) */
  if (node->decorators && !node->generic_params)
    checker_evaluate_decorators(ctx, node->decorators, DECORATOR_TARGET_TYPE,
                                name, (node_t)node);
}

static void _evaluate_cunion(checker_t ctx, cubec_statement_cunion_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_TYPE || !sym->type.type) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  semantic_type_t t = sym->type.type;

  _resolve_struct_fields(ctx, t, node->fields);

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

  /* Resolve parameters using unified helper */
  func_check_info_t info = {
    .arguments = method->arguments,
    .return_type = method->return_type,
    .generic_params = method->generic_params,
  };
  vec_t params = _resolve_func_param_types(ctx, &info);

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

  /* Generic interface: register generic params, store template, skip method resolution */
  if (node->generic_params) {
    checker_register_generic_params(ctx, node->generic_params);
    sym->type.generic_params = node->generic_params;
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

  func_check_info_t info;
  func_check_info_from_statement(&info, node);

  semantic_type_t ftype = _process_function(ctx, &info, &(func_context_t){
      .symbol_scope = NULL,        /* symbol already exists in global_scope */
      .defer_body = true,
      .is_method = false,
      .host_type = NULL,
      .use_child_scope = false,
      .pre_existing_sym = sym,
      .symbol_state = SYMBOL_EVALUATED
  });

  /* inline functions must have a body (comptime already requires body, so skip) */
  if (info.is_inline && !info.is_comptime && !info.body) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         info.location,
                         "inline function '%s' requires a body", name);
    ctx->error_count++;
  }
  /* inline + comptime: inline is silently ignored (comptime needs no inlining) */

  /* Validate builtin against registry */
  if (info.is_builtin) {
    builtin_entry_t be = builtin_table_lookup(ctx->builtin_table, name);
    if (!be) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           info.location,
                           "unknown builtin '%s'", name);
      ctx->error_count++;
    } else if (info.generic_params) {
      /* Generic builtins: use the builtin table's canonical type —
         the AST-resolved type may not exactly match due to generic
         pack/index reconstruction. */
      sym->function.type = be->type;
      sym->is_builtin = true;
    } else if (!semantic_type_equals(ftype, be->type)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           info.location,
                           "builtin '%s' signature mismatch", name);
      ctx->error_count++;
    } else {
      sym->is_builtin = true;
    }
  }

  /* Bind function in comptime env so it can be called at compile time.
     All non-extern non-generic functions with bodies are bound — comptime
     and non-comptime alike. Generic templates are instantiated on demand. */
  if (!info.is_extern && info.body && !info.generic_params) {
    vec_t param_names = NULL;
    if (info.arguments) {
      vec_init_t pvi = {.auto_dispose = false};
      param_names = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &pvi);
      size_t acount = vec_get_size(info.arguments);
      for (size_t i = 0; i < acount; i++) {
        node_t arg = (node_t)vec_get(info.arguments, i);
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
        info.body, param_names, ftype);
    comptime_env_bind_value(ctx->comptime_eval->global_env,
                            ctx->comptime_eval->valloc, name, fn_val);
  }

  /* Evaluate decorators (skip for generic — evaluated at instantiation) */
  if (info.decorators && !info.generic_params)
    checker_evaluate_decorators(ctx, info.decorators, DECORATOR_TARGET_FUNC,
                                name, (node_t)node);
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

  /* 'using' not allowed at module scope */
  if (node->is_using) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "'using' declaration not allowed at module scope");
    ctx->error_count++;
  }

  /* 'using' cannot be initialized with undefined */
  if (node->is_using && decl->expression &&
      decl->expression->kind == CUBEC_NODE_LITERAL_UNDEFINED) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "'using' variable cannot be initialized with undefined");
    ctx->error_count++;
  }

  /* Global comptime variables must be const and cannot use undefined.
     This guarantees the value is known at compile time, so comptime if/for
     etc. never depend on external mutation. */
  if (node->is_comptime && scope_get_kind(ctx->current_scope) == SCOPE_GLOBAL) {
    if (decl->expression &&
        decl->expression->kind == CUBEC_NODE_LITERAL_UNDEFINED) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           node->super.location,
                           "comptime variable '%s' cannot be initialized with 'undefined' — value must be known at compile time",
                           _checker_ident_str(decl->identifier));
      ctx->error_count++;
    }
    if (!decl->expression) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           node->super.location,
                           "comptime variable '%s' requires an initializer — value must be known at compile time",
                           _checker_ident_str(decl->identifier));
      ctx->error_count++;
    }
  }

  const char *name = _checker_ident_str(decl->identifier);
  if (!name) return;

  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_VARIABLE) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  semantic_type_t var_type = NULL;
  semantic_type_t init_type = NULL;

  /* Explicit type annotation */
  if (decl->type)
    var_type = resolver_resolve_type(ctx, decl->type);

  /* Evaluate initializer expression.
     'undefined' is special — it's only valid as a variable initializer
     and means zero-initialization with the declared type. It has no
     standalone type, so we handle it here rather than through
     _check_expression (which would reject it). */
  bool is_undefined_init = decl->expression &&
      decl->expression->kind == CUBEC_NODE_LITERAL_UNDEFINED;

  if (is_undefined_init) {
    /* undefined requires an explicit type annotation */
    if (!var_type) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           node->super.location,
                           "variable '%s' initialized with 'undefined' requires a type annotation",
                           name);
      ctx->error_count++;
      var_type = ctx->error_type;
    }
    /* init_type stays NULL — undefined is compatible with any type */
  } else if (decl->expression) {
    init_type = _check_expression(ctx, decl->expression);
  }

  /* Type inference from initializer when no explicit type */
  if (!var_type && init_type)
    var_type = init_type;

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

  /* Global comptime variables are implicitly const — their value is fixed
     at compile time, so they must not be mutable. This guarantees comptime
     if/for etc. never depend on external mutation. */
  if (node->is_comptime && scope_get_kind(ctx->current_scope) == SCOPE_GLOBAL &&
      var_type && var_type->impl->kind != TYPE_ERROR &&
      !semantic_type_is_const(var_type)) {
    semantic_type_t const_type = semantic_type_create_qualifier(
        ctx->allocator, var_type, true, false);
    type_hash_ensure(const_type);
    vec_push(ctx->all_types, const_type);
    var_type = const_type;
  }

  /* Check initializer type compatibility with explicit type annotation */
  if (var_type && init_type && var_type->impl->kind != TYPE_ERROR &&
      init_type->impl->kind != TYPE_ERROR &&
      !semantic_type_can_implicit_convert(init_type, var_type)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "cannot initialize variable '%s' of type '%s' with value of type '%s'",
                         name,
                         var_type->name ? var_type->name : "<anonymous>",
                         init_type->name ? init_type->name : "<anonymous>");
    ctx->error_count++;
  }

  sym->variable.type = var_type;
  sym->variable.is_comptime = node->is_comptime;
  sym->variable.is_mutable = !semantic_type_is_const(var_type);
  sym->variable.is_using = node->is_using;
  sym->state = SYMBOL_EVALUATED;

  /* 'using' requires the type to implement __dispose__ */
  if (node->is_using && var_type && var_type->impl->kind != TYPE_ERROR) {
    struct symbol *dispose_sym = NULL;
    if (var_type->instance_methods) {
      size_t mc = vec_get_size(var_type->instance_methods);
      for (size_t i = 0; i < mc; i++) {
        struct symbol *m = (struct symbol *)vec_get(var_type->instance_methods, i);
        if (m && m->name && strcmp(m->name, "__dispose__") == 0) {
          dispose_sym = m;
          break;
        }
      }
    }
    if (!dispose_sym) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           node->super.location,
                           "type '%s' must implement '__dispose__' for 'using' declaration",
                           var_type->name ? var_type->name : "<anonymous>");
      ctx->error_count++;
    } else if (dispose_sym->function.type &&
               dispose_sym->function.type->impl->kind == TYPE_FUNCTION) {
      if (dispose_sym->function.type->impl->function.return_type &&
          dispose_sym->function.type->impl->function.return_type->impl->kind != TYPE_VOID) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             node->super.location,
                             "'__dispose__' must return void");
        ctx->error_count++;
      }
    }
  }

  /* Validate builtin variable against registry.
     Since builtin table only contains functions, a builtin var declaration
     will fail with "unknown builtin" if the name doesn't match. */
  if (node->is_builtin) {
    builtin_entry_t be = builtin_table_lookup(ctx->builtin_table, name);
    if (!be) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           node->super.location,
                           "unknown builtin '%s'", name);
      ctx->error_count++;
    } else if (var_type->impl->kind != TYPE_ERROR &&
               !semantic_type_equals(var_type, be->type)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           node->super.location,
                           "builtin '%s' type mismatch", name);
      ctx->error_count++;
    } else {
      sym->is_builtin = true;
    }
  }

  /* Bind comptime-evaluable value to comptime env.
   * For comptime var: always bind.
   * For non-comptime var with decorators: also try to bind (needed for decorator eval). */
  bool need_comptime_binding = node->is_comptime || (node->decorators != NULL);
  if (need_comptime_binding && node->declarator &&
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

  if (node->decorators)
    checker_evaluate_decorators(ctx, node->decorators, DECORATOR_TARGET_VAR,
                                name, (node_t)node);
}

static void _evaluate_type_alias(checker_t ctx,
                                 cubec_statement_declaration_type_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_TYPE) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  /* Register generic params BEFORE resolving type_value,
     so that type expressions can reference the generic parameters. */
  if (node->params) {
    checker_register_generic_params(ctx, node->params);
    sym->type.generic_params = node->params;
  }

  if (node->type_value) {
    semantic_type_t resolved = resolver_resolve_type(ctx, node->type_value);
    sym->type.type = resolved;
  } else if (node->is_builtin) {
    builtin_entry_t be = builtin_table_lookup(ctx->builtin_table, name);
    if (be && !be->eval_call) {
      /* Non-function builtin = type builtin (reserved for future use) */
      sym->type.type = be->type;
    }
  } else {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "type alias '%s' requires a type expression", name);
    ctx->error_count++;
  }

  /* Generic type alias: still mark evaluated (template) */
  sym->state = SYMBOL_EVALUATED;

  /* Validate builtin type against registry */
  if (node->is_builtin) {
    builtin_entry_t be = builtin_table_lookup(ctx->builtin_table, name);
    if (!be) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           node->super.location,
                           "unknown builtin '%s'", name);
      ctx->error_count++;
    } else if (be->eval_call) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           node->super.location,
                           "builtin '%s' is not a type", name);
      ctx->error_count++;
    } else {
      sym->is_builtin = true;
    }
  }

  if (node->decorators)
    checker_evaluate_decorators(ctx, node->decorators, DECORATOR_TARGET_TYPE,
                                name, (node_t)node);
}

/**
 * @brief Lazy-initialize project context on first non-relative import.
 *
 * Finds project root, sets cubec_home, and loads manifest deps.
 */
static void _ensure_project_context(checker_t ctx) {
  if (ctx->project_root) return;  /* already initialized */
  if (!ctx->current_file) return;

  char *root = manifest_find_root(ctx->current_file);
  if (root) {
    ctx->project_root = root;
    if (!ctx->cubec_home) {
      ctx->cubec_home = strdup(root);
    }
    /* Load manifest deps */
    if (!ctx->manifest_deps) {
      strmap_init_t si = {.value_auto_dispose = false};
      ctx->manifest_deps = (strmap_t)allocator_create(ctx->allocator, &g_strmap_type, &si);
      char *proj_name = NULL;
      char **dep_names = NULL;
      if (manifest_parse(root, &proj_name, &dep_names) == 0) {
        if (dep_names) {
          for (int i = 0; dep_names[i]; i++) {
            strmap_insert(ctx->manifest_deps, dep_names[i], (void *)(intptr_t)1);
          }
          manifest_free_dep_names(dep_names);
        }
      }
      free(proj_name);
    }
  } else {
    /* No manifest — single file mode */
    if (!ctx->cubec_home) {
      ctx->cubec_home = strdup(".");
    }
  }
}

static void _evaluate_import(checker_t ctx,
                             cubec_statement_import_t node) {
  const char *name = _checker_ident_str(node->module_name);
  if (!name) return;

  const char *effective_name = node->alias ? _checker_ident_str(node->alias) : name;
  struct symbol *sym = scope_lookup_local(ctx->global_scope, effective_name);
  if (!sym || sym->kind != SYMBOL_MODULE) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  /* Extract path string from the import statement */
  const char *import_path = NULL;
  if (node->path && node->path->kind == CUBEC_NODE_LITERAL_STRING) {
    cubec_literal_string_t path_lit = (cubec_literal_string_t)node->path;
    import_path = string_get(path_lit->value);
  }
  if (!import_path || !*import_path) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->super.location,
                         "import requires a path string");
    ctx->error_count++;
    return;
  }

  /* Resolve the import path */
  _ensure_project_context(ctx);
  bool is_ghost = false;
  char *resolved = module_resolve_import(import_path, ctx->current_file,
                                          ctx->cubec_home, ctx->project_root,
                                          ctx->manifest_deps, &is_ghost);
  if (is_ghost) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->super.location,
                         "dependency '%s' not declared in manifest.json", import_path);
    ctx->error_count++;
  }
  if (!resolved) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->super.location,
                         "cannot resolve import path '%s'", import_path);
    ctx->error_count++;
    return;
  }

  /* Check module_cache: already loaded or in progress (cycle) */
  module_entry_t cached = (module_entry_t)strmap_find(ctx->module_cache, resolved);
  if (cached) {
    sym->module.scope = cached->scope;
    sym->state = SYMBOL_EVALUATED;
    if (cached->state == MODULE_PARSING) {
      /* Circular dependency: symbols from the in-progress module
         are available by name (NAME_KNOWN) but value references
         will trigger TDZ errors — consistent with TDZ semantics. */
    }
    free(resolved);
    return;
  }

  /* Load the module file */
  size_t src_len;
  char *source = module_read_file(resolved, &src_len);
  if (!source) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->super.location,
                         "cannot read module '%s'", import_path);
    ctx->error_count++;
    free(resolved);
    return;
  }

  /* Tokenize */
  vec_t tokens = resolve_token_list(ctx->allocator, resolved, source);
  if (!tokens) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->super.location,
                         "lexing failed for module '%s'", import_path);
    ctx->error_count++;
    free(source);
    free(resolved);
    return;
  }

  /* Parse */
  size_t pos = 0;
  node_t program = read_program_node(ctx->allocator, tokens, &pos, resolved);
  if (!program) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->super.location,
                         "parsing failed for module '%s'", import_path);
    ctx->error_count++;
    free(source);
    free(resolved);
    return;
  }

  /* Create module entry with PARSING state (for cycle detection) */
  module_entry_t entry = module_entry_create(resolved);
  entry->source = source;

  /* Insert into cache BEFORE compiling (enables cycle detection) */
  strmap_insert(ctx->module_cache, resolved, entry);

  /* Compile the imported module in the SAME checker context.
     This ensures all modules share the same builtin types, type_name_table,
     and comptime evaluator — critical for type compatibility across modules.

     The module's global scope is a child of the checker's global_scope
     so that builtin types are visible, but module symbols don't pollute
     the main scope. */
  scope_t mod_scope = scope_create(ctx->allocator, ctx->global_scope,
                                    SCOPE_GLOBAL, node->super.location);
  vec_push(ctx->all_scopes, mod_scope);

  /* Save and swap checker state */
  scope_t saved_scope = ctx->current_scope;
  scope_t saved_global = ctx->global_scope;
  const char *saved_file = ctx->current_file;
  flow_state_t saved_flow = ctx->current_flow;

  /* Switch to module's scope as the "global" scope for compilation.
     The module scope is a child of the real global_scope so builtins
     are accessible via scope_lookup. */
  ctx->global_scope = mod_scope;
  ctx->current_scope = mod_scope;
  ctx->current_file = resolved;
  ctx->current_flow = NULL;

  /* Load source into the source cache for diagnostics */
  source_cache_load(ctx->sources, resolved, source, false);

  /* Compile the imported module using the same checker */
  checker_collect_declarations(ctx, program);
  entry->state = MODULE_PARSED;
  checker_evaluate_declarations(ctx, program);

  /* Run function body checking on the imported module */
  checker_check_function_bodies(ctx, program);

  /* Restore checker state */
  ctx->global_scope = saved_global;
  ctx->current_scope = saved_scope;
  ctx->current_file = saved_file;
  ctx->current_flow = saved_flow;

  /* Update module entry */
  entry->state = MODULE_CHECKED;
  entry->scope = mod_scope;
  entry->checker = NULL; /* no separate checker */
  entry->tokens = tokens;
  entry->program = program;

  /* Link the symbol to the module's scope */
  sym->module.scope = mod_scope;
  sym->state = SYMBOL_EVALUATED;

  free(resolved);
}

/**
 * @brief Load a module by path and return its scope.
 *
 * Shared by _evaluate_import and _evaluate_export_from.
 * Resolves the path, checks cache, loads/compiles if needed.
 * Returns the module's scope, or NULL on failure.
 * The caller must free() the returned resolved path (if out_resolved is non-NULL).
 *
 * @param ctx          Checker context
 * @param import_path  Raw import path string
 * @param location     Location for diagnostics
 * @param out_resolved If non-NULL, receives the malloc'd resolved path (caller frees)
 */
static scope_t _load_module_by_path(checker_t ctx, const char *import_path,
                                     location_t location,
                                     char **out_resolved) {
  _ensure_project_context(ctx);
  bool is_ghost = false;
  char *resolved = module_resolve_import(import_path, ctx->current_file,
                                          ctx->cubec_home, ctx->project_root,
                                          ctx->manifest_deps, &is_ghost);
  if (is_ghost) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, location,
                         "dependency '%s' not declared in manifest.json", import_path);
    ctx->error_count++;
  }
  if (!resolved) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, location,
                         "cannot resolve import path '%s'", import_path);
    ctx->error_count++;
    return NULL;
  }

  /* Check module_cache */
  module_entry_t cached = (module_entry_t)strmap_find(ctx->module_cache, resolved);
  if (cached) {
    if (out_resolved) *out_resolved = resolved; else free(resolved);
    return cached->scope;
  }

  /* Load the module file */
  size_t src_len;
  char *source = module_read_file(resolved, &src_len);
  if (!source) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, location,
                         "cannot read module '%s'", import_path);
    ctx->error_count++;
    free(resolved);
    return NULL;
  }

  /* Tokenize */
  vec_t tokens = resolve_token_list(ctx->allocator, resolved, source);
  if (!tokens) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, location,
                         "lexing failed for module '%s'", import_path);
    ctx->error_count++;
    free(source);
    free(resolved);
    return NULL;
  }

  /* Parse */
  size_t pos = 0;
  node_t program = read_program_node(ctx->allocator, tokens, &pos, resolved);
  if (!program) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, location,
                         "parsing failed for module '%s'", import_path);
    ctx->error_count++;
    free(source);
    free(resolved);
    return NULL;
  }

  /* Create module entry with PARSING state (for cycle detection) */
  module_entry_t entry = module_entry_create(resolved);
  entry->source = source;

  /* Insert into cache BEFORE compiling */
  strmap_insert(ctx->module_cache, resolved, entry);

  /* Compile the module in the SAME checker context */
  scope_t mod_scope = scope_create(ctx->allocator, ctx->global_scope,
                                    SCOPE_GLOBAL, location);
  vec_push(ctx->all_scopes, mod_scope);

  /* Save and swap checker state */
  scope_t saved_scope = ctx->current_scope;
  scope_t saved_global = ctx->global_scope;
  const char *saved_file = ctx->current_file;
  flow_state_t saved_flow = ctx->current_flow;

  ctx->global_scope = mod_scope;
  ctx->current_scope = mod_scope;
  ctx->current_file = resolved;
  ctx->current_flow = NULL;

  source_cache_load(ctx->sources, resolved, source, false);

  checker_collect_declarations(ctx, program);
  entry->state = MODULE_PARSED;
  checker_evaluate_declarations(ctx, program);
  checker_check_function_bodies(ctx, program);

  /* Restore checker state */
  ctx->global_scope = saved_global;
  ctx->current_scope = saved_scope;
  ctx->current_file = saved_file;
  ctx->current_flow = saved_flow;

  /* Update module entry */
  entry->state = MODULE_CHECKED;
  entry->scope = mod_scope;
  entry->checker = NULL;
  entry->tokens = tokens;
  entry->program = program;

  if (out_resolved) *out_resolved = resolved; else free(resolved);
  return mod_scope;
}

/**
 * @brief Create a proxy symbol that re-exports a symbol from another module.
 *
 * The proxy shares the type/ast_node references with the original symbol
 * and is marked is_export=true in the current module's scope.
 */
static struct symbol *_create_proxy_symbol(checker_t ctx,
                                           struct symbol *original,
                                           location_t location) {
  struct symbol *proxy = symbol_create(ctx->allocator, original->name,
                                       original->kind, location);
  proxy->is_export = true;
  proxy->state = original->state;

  /* Copy kind-specific fields */
  switch (original->kind) {
  case SYMBOL_VARIABLE:
    proxy->variable.type = original->variable.type;
    proxy->variable.is_comptime = original->variable.is_comptime;
    proxy->variable.is_mutable = original->variable.is_mutable;
    proxy->variable.is_using = original->variable.is_using;
    break;
  case SYMBOL_FUNCTION:
    proxy->function.type = original->function.type;
    proxy->function.is_comptime = original->function.is_comptime;
    proxy->function.self_param = original->function.self_param;
    proxy->function.ast_node = original->function.ast_node;
    proxy->function.generic_params = original->function.generic_params;
    break;
  case SYMBOL_TYPE:
    proxy->type.type = original->type.type;
    proxy->type.generic_params = original->type.generic_params;
    break;
  case SYMBOL_ENUM_ITEM:
    proxy->enum_item.value = original->enum_item.value;
    proxy->enum_item.owning_type = original->enum_item.owning_type;
    break;
  default:
    break;
  }
  return proxy;
}

static void _evaluate_export_from(checker_t ctx,
                                  cubec_statement_export_from_t node) {
  /* Extract path string */
  const char *import_path = NULL;
  if (node->path && node->path->kind == CUBEC_NODE_LITERAL_STRING) {
    cubec_literal_string_t path_lit = (cubec_literal_string_t)node->path;
    import_path = string_get(path_lit->value);
  }
  if (!import_path || !*import_path) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->super.location,
                         "export from requires a path string");
    ctx->error_count++;
    return;
  }

  /* Load the target module */
  scope_t mod_scope = _load_module_by_path(ctx, import_path, node->super.location, NULL);
  if (!mod_scope) return;

  if (node->is_star) {
    /* export * from "path" — re-export all exported symbols */
    vec_t symbols = scope_get_symbols(mod_scope);
    size_t count = vec_get_size(symbols);
    for (size_t i = 0; i < count; i++) {
      struct symbol *sym = (struct symbol *)vec_get(symbols, i);
      if (!sym->is_export) continue;

      /* Skip if already declared in current scope */
      if (scope_lookup_local(ctx->global_scope, sym->name)) continue;

      struct symbol *proxy = _create_proxy_symbol(ctx, sym, node->super.location);
      scope_push_symbol(ctx->global_scope, proxy);
    }
  } else {
    /* export { a, b } from "path" — selective re-export */
    if (!node->names) return;
    size_t ncount = vec_get_size(node->names);
    for (size_t i = 0; i < ncount; i++) {
      node_t name_node = (node_t)vec_get(node->names, i);
      const char *name = _checker_ident_str(name_node);
      if (!name) continue;

      /* Look up in target module's scope */
      struct symbol *original = scope_lookup_local(mod_scope, name);
      if (!original) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             name_node->location,
                             "module '%s' has no member '%s'",
                             import_path, name);
        ctx->error_count++;
        continue;
      }
      if (!original->is_export) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             name_node->location,
                             "'%s' is not exported from module '%s'",
                             name, import_path);
        ctx->error_count++;
        continue;
      }

      /* Check for duplicate in current scope */
      if (scope_lookup_local(ctx->global_scope, name)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             name_node->location,
                             "duplicate declaration of '%s'", name);
        ctx->error_count++;
        continue;
      }

      struct symbol *proxy = _create_proxy_symbol(ctx, original, node->super.location);
      scope_push_symbol(ctx->global_scope, proxy);
    }
  }
}

static void _evaluate_comptime_if(checker_t ctx,
                                  cubec_statement_comptime_if_t node) {
  if (!ctx->comptime_eval) return;

  /* Evaluate condition — must be compile-time bool */
  comptime_value_t cond =
      comptime_eval_expr(ctx->comptime_eval, ctx, node->condition);
  if (!cond || _val_is_error(cond)) {
    if (cond && cond->kind == COMPTIME_VALUE_FATAL) {
      ctx->fatal_error = true;
      return;
    }
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "comptime if condition must be a compile-time bool");
    ctx->error_count++;
    return;
  }
  if (cond->kind != COMPTIME_VALUE_BOOL) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "comptime if condition must be a compile-time bool");
    ctx->error_count++;
    return;
  }

  /* Determine taken branch */
  bool condition_true = comptime_value_is_truthy(cond);
  node_t taken = condition_true ? node->then_branch : node->else_branch;

  /* Collect and evaluate declarations in the taken branch only */
  if (taken) {
    if (taken->kind == CUBEC_NODE_STATEMENT_BLOCK) {
      cubec_statement_block_t blk = (cubec_statement_block_t)taken;
      if (blk->statements) {
        size_t count = vec_get_size(blk->statements);
        for (size_t i = 0; i < count; i++) {
          if (ctx->fatal_error) break;
          node_t s = (node_t)vec_get(blk->statements, i);
          checker_collect_statement(ctx, s);
        }
        for (size_t i = 0; i < count; i++) {
          if (ctx->fatal_error) break;
          node_t s = (node_t)vec_get(blk->statements, i);
          checker_evaluate_statement(ctx, s);
        }
      }
    }
  }

  /* Execute the taken branch at comptime */
  comptime_signal_t sig =
      comptime_eval_exec_comptime_if(ctx->comptime_eval, ctx, (node_t)node);
  if (sig.kind == COMPTIME_SIGNAL_FATAL)
    ctx->fatal_error = true;
  else if (sig.kind == COMPTIME_SIGNAL_ERROR)
    ctx->error_count++;
}

static void _evaluate_comptime_foreach(checker_t ctx,
                                       cubec_statement_comptime_foreach_t node) {
  if (!ctx->comptime_eval) return;
  comptime_signal_t sig =
      comptime_eval_exec_comptime_foreach(ctx->comptime_eval, ctx, (node_t)node);
  if (sig.kind == COMPTIME_SIGNAL_FATAL)
    ctx->fatal_error = true;
  else if (sig.kind == COMPTIME_SIGNAL_ERROR) ctx->error_count++;
}

static void _evaluate_test(checker_t ctx,
                           cubec_statement_test_t node) {
  if (!ctx->comptime_eval) return;

  {
    FILE *dbg = fopen("C:/tmp/cubec_debug.txt", "a");
    if (dbg) { fprintf(dbg, "ENTER _evaluate_test\n"); fflush(dbg); fclose(dbg); }
  }

  /* Check the test body for type errors before evaluating.
   * Set up current_flow so TDZ tracking works inside test bodies.
   * Set in_test_block so assert() is allowed in checker. */
  ctx->in_test_block = true;
  int errors_before_check = ctx->error_count;
  flow_state_t saved_flow = ctx->current_flow;
  {
    FILE *dbg = fopen("C:/tmp/cubec_debug.txt", "a");
    if (dbg) { fprintf(dbg, "BEFORE _check_statement\n"); fflush(dbg); fclose(dbg); }
  }
  flow_state_t fs = _check_statement(ctx, node->body, NULL);
  {
    FILE *dbg = fopen("C:/tmp/cubec_debug.txt", "a");
    if (dbg) { fprintf(dbg, "AFTER _check_statement\n"); fflush(dbg); fclose(dbg); }
  }
  ctx->current_flow = saved_flow;
  flow_state_dispose(fs, ctx->allocator);
  ctx->in_test_block = false;

  ctx->test_count++;

  /* If checker found errors, skip eval and mark test as failed */
  if (ctx->error_count > errors_before_check) {
    ctx->test_fail_count++;
    return;
  }

  /* Evaluate with in_test_block set so assert failure is non-fatal */
  ctx->comptime_eval->in_test_block = true;
  int errors_before_eval = ctx->error_count;
  {
    FILE *dbg = fopen("C:/tmp/cubec_debug.txt", "a");
    if (dbg) { fprintf(dbg, "BEFORE comptime_eval_exec_block\n"); fflush(dbg); fclose(dbg); }
  }
  comptime_signal_t sig =
      comptime_eval_exec_block(ctx->comptime_eval, ctx, node->body);
  {
    FILE *dbg = fopen("C:/tmp/cubec_debug.txt", "a");
    if (dbg) { fprintf(dbg, "AFTER comptime_eval_exec_block\n"); fflush(dbg); fclose(dbg); }
  }
  ctx->comptime_eval->in_test_block = false;

  if (sig.kind == COMPTIME_SIGNAL_FATAL) {
    /* panic inside test: fatal — stop compilation */
    ctx->fatal_error = true;
    return;
  }
  if (sig.kind == COMPTIME_SIGNAL_ERROR || ctx->error_count > errors_before_eval) {
    ctx->test_fail_count++;
  }
}

void checker_evaluate_declarations(checker_t ctx, node_t program) {
  cubec_program_node_t prog = (cubec_program_node_t)program;
  if (!prog || !prog->statements) return;

  size_t count = vec_get_size(prog->statements);
  for (size_t i = 0; i < count; i++) {
    if (ctx->fatal_error) break;
    node_t stmt = (node_t)vec_get(prog->statements, i);
    if (!stmt) continue;
    checker_evaluate_statement(ctx, stmt);
  }
}

void checker_evaluate_statement(checker_t ctx, node_t stmt) {
  if (!stmt) return;

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
  case CUBEC_NODE_STATEMENT_EXPORT_FROM:     _evaluate_export_from(ctx, (cubec_statement_export_from_t)stmt); break;
  case CUBEC_NODE_STATEMENT_COMPTIME_IF:     _evaluate_comptime_if(ctx, (cubec_statement_comptime_if_t)stmt); break;
  case CUBEC_NODE_STATEMENT_COMPTIME_FOREACH: _evaluate_comptime_foreach(ctx, (cubec_statement_comptime_foreach_t)stmt); break;
  case CUBEC_NODE_STATEMENT_TEST:            _evaluate_test(ctx, (cubec_statement_test_t)stmt); break;
  default: break;
  }
}
