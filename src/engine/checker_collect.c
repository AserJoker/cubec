#include "engine/context.h"
#include "engine/checker_collect.h"
#include "engine/checker_func_util.h"
#include "engine/checker_type_util.h"
#include "engine/resolver.h"
#include "cubec/program.h"
#include "cubec/literal_identifier.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_enum.h"
#include "cubec/statement_union.h"
#include "cubec/statement_cunion.h"
#include "cubec/statement_interface.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/statement_import.h"
#include "cubec/statement_export_from.h"
#include "cubec/declaration_variable.h"
#include "cubec/generic_param.h"

/* _register_generic_params moved to checker_func_util.c as context_register_generic_params */

/** Set source_file on a type for cross-module pub visibility checks. */
static inline void _set_source_file(semantic_type_t t, const char *current_file) {
  if (t) t->source_file = current_file;
}

static void _collect_struct(context_t ctx, cubec_statement_struct_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  if (scope_lookup_local(ctx->global_scope, name)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "duplicate declaration of '%s'", name);
    ctx->error_count++;
    return;
  }

  semantic_type_t t =
      semantic_type_create_named(ctx->allocator, name, TYPE_STRUCT);
  _set_source_file(t, ctx->current_file);
  vec_push(ctx->all_types, t);

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->is_export = node->is_export;
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);

  strmap_insert(ctx->type_name_table, name, t);
}

static void _collect_enum(context_t ctx, cubec_statement_enum_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  if (scope_lookup_local(ctx->global_scope, name)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "duplicate declaration of '%s'", name);
    ctx->error_count++;
    return;
  }

  semantic_type_t t =
      semantic_type_create_named(ctx->allocator, name, TYPE_ENUM);
  _set_source_file(t, ctx->current_file);
  vec_push(ctx->all_types, t);

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->is_export = node->is_export;
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);

  strmap_insert(ctx->type_name_table, name, t);
}

static void _collect_union(context_t ctx, cubec_statement_union_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  if (scope_lookup_local(ctx->global_scope, name)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "duplicate declaration of '%s'", name);
    ctx->error_count++;
    return;
  }

  semantic_type_t t =
      semantic_type_create_named(ctx->allocator, name, TYPE_UNION);
  _set_source_file(t, ctx->current_file);
  vec_push(ctx->all_types, t);

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->is_export = node->is_export;
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);

  strmap_insert(ctx->type_name_table, name, t);
}

static void _collect_cunion(context_t ctx, cubec_statement_cunion_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  if (scope_lookup_local(ctx->global_scope, name)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "duplicate declaration of '%s'", name);
    ctx->error_count++;
    return;
  }

  semantic_type_t t =
      semantic_type_create_named(ctx->allocator, name, TYPE_CUNION);
  _set_source_file(t, ctx->current_file);
  vec_push(ctx->all_types, t);

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);

  strmap_insert(ctx->type_name_table, name, t);
}

static void _collect_interface(context_t ctx,
                                cubec_statement_interface_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  if (scope_lookup_local(ctx->global_scope, name)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "duplicate declaration of '%s'", name);
    ctx->error_count++;
    return;
  }

  semantic_type_t t =
      semantic_type_create_named(ctx->allocator, name, TYPE_INTERFACE);
  _set_source_file(t, ctx->current_file);
  t->is_interface = true;
  vec_push(ctx->all_types, t);

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->is_export = node->is_export;
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);

  strmap_insert(ctx->type_name_table, name, t);
}

static void _collect_function(context_t ctx,
                               cubec_statement_function_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  if (scope_lookup_local(ctx->global_scope, name)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "duplicate declaration of '%s'", name);
    ctx->error_count++;
    return;
  }

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_FUNCTION,
                    node->super.location);
  sym->is_export = node->is_export;
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);
}

static void _collect_variable(context_t ctx,
                               cubec_statement_declaration_t node) {
  cubec_declaration_variable_t decl =
      (cubec_declaration_variable_t)node->declarator;
  if (!decl) return;

  const char *name = _checker_ident_str(decl->identifier);
  if (!name) return;

  if (scope_lookup_local(ctx->global_scope, name)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "duplicate declaration of '%s'", name);
    ctx->error_count++;
    return;
  }

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_VARIABLE,
                    node->super.location);
  sym->is_export = node->is_export;
  sym->variable.is_comptime = node->is_comptime;
  sym->variable.is_mutable = true;
  scope_push_symbol(ctx->global_scope, sym);
}

static void _collect_type_alias(context_t ctx,
                                 cubec_statement_declaration_type_t node) {
  const char *name = _checker_ident_str(node->name);
  if (!name) return;

  if (scope_lookup_local(ctx->global_scope, name)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "duplicate declaration of '%s'", name);
    ctx->error_count++;
    return;
  }

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_TYPE, node->super.location);
  sym->is_export = node->is_export;
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);
}

static void _collect_import(context_t ctx,
                             cubec_statement_import_t node) {
  const char *name = _checker_ident_str(node->module_name);
  if (!name) return;

  if (scope_lookup_local(ctx->global_scope, name)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "duplicate declaration of '%s'", name);
    ctx->error_count++;
    return;
  }

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_MODULE,
                    node->super.location);
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);
}

/**
 * @brief Collect phase for export-from statement.
 *
 * Re-export does not create a SYMBOL_MODULE in the current scope.
 * Actual symbol proxying happens in the evaluate phase after
 * the target module is loaded. This is a no-op placeholder.
 */
static void _collect_export_from(context_t ctx,
                                  cubec_statement_export_from_t node) {
  (void)ctx;
  (void)node;
  /* Nothing to do in collect phase — symbol proxying is deferred to evaluate */
}

void context_collect_declarations(context_t ctx, node_t program) {
  cubec_program_node_t prog = (cubec_program_node_t)program;
  if (!prog || !prog->statements) return;

  size_t count = vec_get_size(prog->statements);
  for (size_t i = 0; i < count; i++) {
    node_t stmt = (node_t)vec_get(prog->statements, i);
    if (!stmt) continue;
    context_collect_statement(ctx, stmt);
  }
}

void context_collect_statement(context_t ctx, node_t stmt) {
  if (!stmt) return;

  switch (stmt->kind) {
  case CUBEC_NODE_STATEMENT_STRUCT:
    _collect_struct(ctx, (cubec_statement_struct_t)stmt);
    break;
  case CUBEC_NODE_STATEMENT_ENUM:
    _collect_enum(ctx, (cubec_statement_enum_t)stmt);
    break;
  case CUBEC_NODE_STATEMENT_UNION:
    _collect_union(ctx, (cubec_statement_union_t)stmt);
    break;
  case CUBEC_NODE_STATEMENT_CUNION:
    _collect_cunion(ctx, (cubec_statement_cunion_t)stmt);
    break;
  case CUBEC_NODE_STATEMENT_INTERFACE:
    _collect_interface(ctx, (cubec_statement_interface_t)stmt);
    break;
  case CUBEC_NODE_STATEMENT_FUNCTION:
    _collect_function(ctx, (cubec_statement_function_t)stmt);
    break;
  case CUBEC_NODE_STATEMENT_DECLARATION:
    _collect_variable(ctx, (cubec_statement_declaration_t)stmt);
    break;
  case CUBEC_NODE_STATEMENT_DECLARATION_TYPE:
    _collect_type_alias(ctx, (cubec_statement_declaration_type_t)stmt);
    break;
  case CUBEC_NODE_STATEMENT_IMPORT:
    _collect_import(ctx, (cubec_statement_import_t)stmt);
    break;
  case CUBEC_NODE_STATEMENT_EXPORT_FROM:
    _collect_export_from(ctx, (cubec_statement_export_from_t)stmt);
    break;
  case CUBEC_NODE_STATEMENT_ERROR:
  case CUBEC_NODE_ERROR:
    /* Parse error — diagnostic already recorded. Nothing to collect. */
    break;
  default:
    break;
  }
}
