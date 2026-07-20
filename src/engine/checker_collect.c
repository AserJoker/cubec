#include "engine/checker.h"
#include "engine/checker_collect.h"
#include "engine/checker_func_util.h"
#include "engine/checker_type_util.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"
#include "cubec/node.h"
#include "cubec/program.h"
#include "cubec/literal_identifier.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_enum.h"
#include "cubec/statement_union.h"
#include "cubec/statement_cunion.h"
#include "cubec/statement_interface.h"
#include "cubec/statement_function.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/statement_import.h"
#include "cubec/declaration_variable.h"
#include "cubec/generic_param.h"

/* _register_generic_params moved to checker_func_util.c as checker_register_generic_params */

static void _collect_struct(checker_t ctx, cubec_statement_struct_t node) {
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
  vec_push(ctx->all_types, t);

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);

  strmap_insert(ctx->type_name_table, name, t);
}

static void _collect_enum(checker_t ctx, cubec_statement_enum_t node) {
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
  vec_push(ctx->all_types, t);

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);

  strmap_insert(ctx->type_name_table, name, t);
}

static void _collect_union(checker_t ctx, cubec_statement_union_t node) {
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
  vec_push(ctx->all_types, t);

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);

  strmap_insert(ctx->type_name_table, name, t);
}

static void _collect_cunion(checker_t ctx, cubec_statement_cunion_t node) {
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
  vec_push(ctx->all_types, t);

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);

  strmap_insert(ctx->type_name_table, name, t);
}

static void _collect_interface(checker_t ctx,
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
  t->is_interface = true;
  vec_push(ctx->all_types, t);

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);

  strmap_insert(ctx->type_name_table, name, t);
}

static void _collect_function(checker_t ctx,
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
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);
}

static void _collect_variable(checker_t ctx,
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
  sym->variable.is_comptime = node->is_comptime;
  sym->variable.is_mutable = true;
  scope_push_symbol(ctx->global_scope, sym);
}

static void _collect_type_alias(checker_t ctx,
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
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);
}

static void _collect_import(checker_t ctx,
                             cubec_statement_import_t node) {
  const char *name = _checker_ident_str(node->module_name);
  if (!name) return;

  const char *effective_name = node->alias ? _checker_ident_str(node->alias) : name;

  if (scope_lookup_local(ctx->global_scope, effective_name)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "duplicate declaration of '%s'", effective_name);
    ctx->error_count++;
    return;
  }

  struct symbol *sym =
      symbol_create(ctx->allocator, effective_name, SYMBOL_MODULE,
                    node->super.location);
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);
}

void checker_collect_declarations(checker_t ctx, node_t program) {
  cubec_program_node_t prog = (cubec_program_node_t)program;
  if (!prog || !prog->statements) return;

  size_t count = vec_get_size(prog->statements);
  for (size_t i = 0; i < count; i++) {
    node_t stmt = (node_t)vec_get(prog->statements, i);
    if (!stmt) continue;

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
    default:
      break;
    }
  }
}
