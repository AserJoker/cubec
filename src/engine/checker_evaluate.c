#include "engine/checker.h"
#include "engine/checker_evaluate.h"
#include "engine/checker_collect.h"
#include "engine/checker_func_util.h"
#include "engine/checker_type_util.h"
#include "engine/checker_check_expr.h"
#include "engine/checker_check_stmt.h"
#include "engine/comptime_eval.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
#include "engine/module.h"
#include "core/allocator.h"
#include "cubec/token.h"
#include "cubec/statement_test.h"
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
                                     cubec_statement_function_t mfn) {
  const char *mname = _checker_ident_str(mfn->name);
  struct symbol *msym = symbol_create(ctx->allocator, mname,
                                      SYMBOL_FUNCTION, mfn->super.location);
  func_check_info_t info;
  func_check_info_from_statement(&info, mfn);
  semantic_type_t ret_type = info.return_type
      ? resolver_resolve_type(ctx, info.return_type)
      : ctx->builtin_void;
  if (!ret_type) ret_type = ctx->builtin_void;
  vec_t params = _resolve_func_param_types(ctx, &info);
  semantic_type_t mtype = semantic_type_create_function(
      ctx->allocator, ret_type, params, info.is_c_variadic);
  type_hash_ensure(mtype);
  vec_push(ctx->all_types, mtype);
  msym->function.type = mtype;
  msym->function.is_comptime = info.is_comptime;
  msym->function.ast_node = info.ast_node;
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
  vsym->variable.is_mutable = !semantic_type_is_const(vsym->variable.type);
  vsym->state = SYMBOL_NAME_KNOWN; /* initializer in Pass 3 */
  vec_push(t->static_fields, vsym);
}

void checker_evaluate_struct_union_members(checker_t ctx, semantic_type_t t,
                                           vec_t members) {
  if (!members) return;
  if (!t->instance_methods) return;
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

  /* Generic struct: register generic params and store template */
  if (node->generic_params) {
    checker_register_generic_params(ctx, node->generic_params);
    sym->type.generic_params = node->generic_params;
  }

  /* Resolve fields and methods */
  _resolve_struct_fields(ctx, t, node->members);
  checker_evaluate_struct_union_members(ctx, t, node->members);

  /* Skip layout for generic — sizes depend on concrete type args */
  if (!node->generic_params) type_layout_compute(t, 8);
  type_hash_ensure(t);
  sym->state = SYMBOL_EVALUATED;
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
  checker_evaluate_struct_union_members(ctx, t, node->members);

  /* Skip layout for generic — sizes depend on concrete type args */
  if (!node->generic_params) type_layout_compute(t, 8);
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

  /* Register generic params if present */
  if (info.generic_params) {
    checker_register_generic_params(ctx, info.generic_params);
    sym->function.generic_params = info.generic_params;
  }

  /* Resolve return type and parameter types using unified helpers */
  semantic_type_t ret_type = info.return_type
      ? resolver_resolve_type(ctx, info.return_type)
      : ctx->builtin_void;
  vec_t params = _resolve_func_param_types(ctx, &info);

  semantic_type_t ftype = semantic_type_create_function(
      ctx->allocator, ret_type, params, info.is_c_variadic);
  type_hash_ensure(ftype);
  vec_push(ctx->all_types, ftype);

  sym->function.type = ftype;
  sym->function.is_comptime = info.is_comptime;
  sym->function.ast_node = info.ast_node;
  sym->state = SYMBOL_EVALUATED;

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
     Only for non-generic functions with bodies — generic templates are
     instantiated on demand. */
  if (info.body && !info.generic_params) {
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

  /* Register generic params for generic type alias */
  if (node->params) {
    checker_register_generic_params(ctx, node->params);
    sym->type.generic_params = node->params;
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

  /* Resolve the import path relative to the current file */
  char *resolved = module_resolve_path(import_path, ctx->current_file);
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

static void _evaluate_comptime_block(checker_t ctx,
                                     cubec_statement_comptime_block_t node) {
  if (!ctx->comptime_eval) return;
  comptime_signal_t sig =
      comptime_eval_exec_block(ctx->comptime_eval, ctx, node->body);
  if (sig.kind == COMPTIME_SIGNAL_FATAL)
    ctx->fatal_error = true;
}

static void _evaluate_comptime_if(checker_t ctx,
                                  cubec_statement_comptime_if_t node) {
  if (!ctx->comptime_eval) return;
  comptime_signal_t sig =
      comptime_eval_exec_comptime_if(ctx->comptime_eval, ctx, (node_t)node);
  if (sig.kind == COMPTIME_SIGNAL_FATAL)
    ctx->fatal_error = true;
  else if (sig.kind == COMPTIME_SIGNAL_ERROR) ctx->error_count++;
}

static void _evaluate_comptime_for(checker_t ctx,
                                   cubec_statement_comptime_for_t node) {
  if (!ctx->comptime_eval) return;
  comptime_signal_t sig =
      comptime_eval_exec_comptime_for(ctx->comptime_eval, ctx, (node_t)node);
  if (sig.kind == COMPTIME_SIGNAL_FATAL)
    ctx->fatal_error = true;
  else if (sig.kind == COMPTIME_SIGNAL_ERROR) ctx->error_count++;
}

static void _evaluate_test(checker_t ctx,
                           cubec_statement_test_t node) {
  if (!ctx->comptime_eval) return;

  /* Check the test body for type errors before evaluating.
   * Set up current_flow so TDZ tracking works inside test bodies.
   * Set in_test_block so assert() is allowed in checker. */
  ctx->in_test_block = true;
  int errors_before_check = ctx->error_count;
  flow_state_t saved_flow = ctx->current_flow;
  flow_state_t fs = _check_statement(ctx, node->body, NULL);
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
  comptime_signal_t sig =
      comptime_eval_exec_block(ctx->comptime_eval, ctx, node->body);
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
    if (ctx->fatal_error) break;  /* panic stops all evaluation */
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
