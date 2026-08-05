#include "engine/name_collector.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "core/diagnostic.h"
#include "core/string.h"
#include "core/strmap.h"
#include "core/vec.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "cubec/node.h"
#include "cubec/program.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_string.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_function.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_union.h"
#include "cubec/statement_enum.h"
#include "cubec/statement_interface.h"
#include "cubec/statement_cunion.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/statement_import.h"
#include "cubec/statement_export.h"
#include "cubec/declaration_variable.h"

/* --------------------------------------------------------------------------
 *  Helpers
 * -------------------------------------------------------------------------- */

/** Extract the C string from an identifier node. */
static const char *_get_identifier_name(node_t node) {
  if (!node)
    return NULL;
  if (node->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
    cubec_literal_identifier_t id = (cubec_literal_identifier_t)node;
    return string_get(id->value);
  }
  return NULL;
}

/** Extract the C string from a string literal node. */
static const char *_get_string_value(node_t node)
    __attribute__((unused));
static const char *_get_string_value(node_t node) {
  if (!node)
    return NULL;
  if (node->kind == CUBEC_NODE_LITERAL_STRING) {
    cubec_literal_string_t str = (cubec_literal_string_t)node;
    return string_get(str->value);
  }
  return NULL;
}

/** Report an error diagnostic at the given node's location. */
static void _report_error(context_t ctx, node_t node, const char *fmt, ...) {
  location_t loc = {0};
  if (node)
    loc = ((struct _node_t *)node)->location;
  va_list args;
  va_start(args, fmt);
  char buf[512];
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, loc, "%s", buf);
}

/** Insert a name into a scope's name table. Returns true on success. */
static bool _scope_insert_name(context_t ctx, scope_t scope, node_t node,
                               const char *name_str, enum name_kind kind,
                               void *ref) {
  name_t existing = (name_t)strmap_find(scope->names, name_str);
  if (existing) {
    _report_error(ctx, node, "duplicate declaration of '%s'", name_str);
    return false;
  }
  name_t name = name_create(scope->allocator, kind, ref);
  strmap_insert(scope->names, name_str, name);
  return true;
}

/** Insert a name into a module's export table. Returns true on success. */
static bool _module_export_name(context_t ctx, module_t mod, node_t node,
                                const char *name_str, enum name_kind kind,
                                void *ref) {
  name_t existing = (name_t)strmap_find(mod->exports, name_str);
  if (existing) {
    _report_error(ctx, node, "duplicate export of '%s'", name_str);
    return false;
  }
  name_t name = name_create(mod->allocator, kind, ref);
  strmap_insert(mod->exports, name_str, name);
  return true;
}

/* --------------------------------------------------------------------------
 *  Statement-level name collection
 * -------------------------------------------------------------------------- */

/**
 * @brief Collect names from a single top-level statement node.
 *
 * For static scopes (module, type): collects var/const, func, type names.
 * For non-static scopes: collects only func and type names (no variable
 * hoisting).
 */
static void _collect_statement(context_t ctx, scope_t scope, node_t stmt,
                               bool is_static_scope) {
  /* Get the owning module for export registration (NULL for non-module scopes) */
  module_t mod = (scope->kind == SCOPE_MODULE && scope->owner)
                     ? (module_t)scope->owner
                     : NULL;

  switch (stmt->kind) {
  /* var / const declarations — only in static scopes */
  case CUBEC_NODE_STATEMENT_DECLARATION: {
    if (!is_static_scope)
      break;
    cubec_statement_declaration_t decl = (cubec_statement_declaration_t)stmt;
    if (decl->is_export && !mod) {
      _report_error(ctx, stmt, "'export' can only be used at module scope");
    }
    if (!decl->declarator)
      break;
    cubec_declaration_variable_t var =
        (cubec_declaration_variable_t)decl->declarator;
    const char *name_str = _get_identifier_name(var->identifier);
    if (!name_str)
      break;
    if (!_scope_insert_name(ctx, scope, stmt, name_str, NAME_VARIABLE, stmt))
      break;
    if (mod && decl->is_export)
      _module_export_name(ctx, mod, stmt, name_str, NAME_VARIABLE, stmt);
    break;
  }

  /* func declarations */
  case CUBEC_NODE_STATEMENT_FUNCTION: {
    cubec_statement_function_t func = (cubec_statement_function_t)stmt;
    if (func->is_export && !mod) {
      _report_error(ctx, stmt, "'export' can only be used at module scope");
    }
    const char *name_str = _get_identifier_name(func->name);
    if (!name_str)
      break;
    if (!_scope_insert_name(ctx, scope, stmt, name_str, NAME_FUNCTION, stmt))
      break;
    if (mod && func->is_export)
      _module_export_name(ctx, mod, stmt, name_str, NAME_FUNCTION, stmt);
    break;
  }

  /* type declarations: struct, union, enum, interface, type alias, cunion */
  case CUBEC_NODE_STATEMENT_STRUCT: {
    cubec_statement_struct_t s = (cubec_statement_struct_t)stmt;
    if (s->is_export && !mod) {
      _report_error(ctx, stmt, "'export' can only be used at module scope");
    }
    const char *name_str = _get_identifier_name(s->name);
    if (!name_str)
      break;
    if (!_scope_insert_name(ctx, scope, stmt, name_str, NAME_TYPE, stmt))
      break;
    if (mod && s->is_export)
      _module_export_name(ctx, mod, stmt, name_str, NAME_TYPE, stmt);
    /* TODO: recurse into struct members for method/type collection */
    break;
  }
  case CUBEC_NODE_STATEMENT_UNION: {
    cubec_statement_union_t u = (cubec_statement_union_t)stmt;
    if (u->is_export && !mod) {
      _report_error(ctx, stmt, "'export' can only be used at module scope");
    }
    const char *name_str = _get_identifier_name(u->name);
    if (!name_str)
      break;
    if (!_scope_insert_name(ctx, scope, stmt, name_str, NAME_TYPE, stmt))
      break;
    if (mod && u->is_export)
      _module_export_name(ctx, mod, stmt, name_str, NAME_TYPE, stmt);
    /* TODO: recurse into union members for method/type collection */
    break;
  }
  case CUBEC_NODE_STATEMENT_ENUM: {
    cubec_statement_enum_t e = (cubec_statement_enum_t)stmt;
    if (e->is_export && !mod) {
      _report_error(ctx, stmt, "'export' can only be used at module scope");
    }
    const char *name_str = _get_identifier_name(e->name);
    if (!name_str)
      break;
    if (!_scope_insert_name(ctx, scope, stmt, name_str, NAME_TYPE, stmt))
      break;
    if (mod && e->is_export)
      _module_export_name(ctx, mod, stmt, name_str, NAME_TYPE, stmt);
    break;
  }
  case CUBEC_NODE_STATEMENT_INTERFACE: {
    cubec_statement_interface_t iface = (cubec_statement_interface_t)stmt;
    if (iface->is_export && !mod) {
      _report_error(ctx, stmt, "'export' can only be used at module scope");
    }
    const char *name_str = _get_identifier_name(iface->name);
    if (!name_str)
      break;
    if (!_scope_insert_name(ctx, scope, stmt, name_str, NAME_TYPE, stmt))
      break;
    if (mod && iface->is_export)
      _module_export_name(ctx, mod, stmt, name_str, NAME_TYPE, stmt);
    /* TODO: recurse into interface members for method/type collection */
    break;
  }
  case CUBEC_NODE_STATEMENT_CUNION: {
    cubec_statement_cunion_t cu = (cubec_statement_cunion_t)stmt;
    const char *name_str = _get_identifier_name(cu->name);
    if (!name_str)
      break;
    _scope_insert_name(ctx, scope, stmt, name_str, NAME_TYPE, stmt);
    break;
  }
  case CUBEC_NODE_STATEMENT_DECLARATION_TYPE: {
    cubec_statement_declaration_type_t t =
        (cubec_statement_declaration_type_t)stmt;
    if (t->is_export && !mod) {
      _report_error(ctx, stmt, "'export' can only be used at module scope");
    }
    const char *name_str = _get_identifier_name(t->name);
    if (!name_str)
      break;
    if (!_scope_insert_name(ctx, scope, stmt, name_str, NAME_TYPE, stmt))
      break;
    if (mod && t->is_export)
      _module_export_name(ctx, mod, stmt, name_str, NAME_TYPE, stmt);
    break;
  }

  /* import — load dependency module and register namespace */
  case CUBEC_NODE_STATEMENT_IMPORT: {
    cubec_statement_import_t imp = (cubec_statement_import_t)stmt;
    if (scope->kind != SCOPE_MODULE) {
      _report_error(ctx, stmt, "'import' can only be used at module scope");
      break;
    }
    const char *mod_name = _get_identifier_name(imp->module_name);
    const char *mod_path = _get_string_value(imp->path);
    if (!mod_name) {
      _report_error(ctx, stmt, "import statement is missing module name");
      break;
    }
    if (!mod_path) {
      _report_error(ctx, stmt, "import statement is missing module path");
      break;
    }
    /* Import the dependency module (read → tokenize → parse → create) */
    module_t dep_mod = context_import(ctx, mod_path);
    if (!dep_mod) {
      _report_error(ctx, stmt, "cannot import module '%s' from '%s'",
                    mod_name, mod_path);
      break;
    }
    /* Recursively run name collection on the dependency if not done */
    if (dep_mod->state == MODULE_NEW) {
      dep_mod->state = MODULE_COLLECTING;
      name_collector_run(ctx, dep_mod);
      dep_mod->state = MODULE_COLLECTED;
    }
    /* Register the module as a namespace in the current scope */
    _scope_insert_name(ctx, scope, stmt, mod_name, NAME_NAMESPACE, dep_mod);
    break;
  }

  /* export — re-export names from dependency module */
  case CUBEC_NODE_STATEMENT_EXPORT: {
    cubec_statement_export_t exp = (cubec_statement_export_t)stmt;
    if (!mod) {
      _report_error(ctx, stmt, "'export' statement can only be used at module scope");
      break;
    }
    const char *mod_path = _get_string_value(exp->path);
    if (!mod_path) {
      _report_error(ctx, stmt, "export statement is missing module path");
      break;
    }
    module_t dep_mod = context_import(ctx, mod_path);
    if (!dep_mod) {
      _report_error(ctx, stmt, "cannot export from module '%s'", mod_path);
      break;
    }
    if (dep_mod->state == MODULE_NEW) {
      dep_mod->state = MODULE_COLLECTING;
      name_collector_run(ctx, dep_mod);
      dep_mod->state = MODULE_COLLECTED;
    }
    /* Copy exported names from dependency into current module's exports */
    if (exp->is_star) {
      /* export * — re-export all */
      strmap_iter_t it = strmap_iter_first(dep_mod->exports);
      const char *key;
      while ((key = strmap_iter_next(&it)) != NULL) {
        name_t value = (name_t)strmap_find(dep_mod->exports, key);
        _module_export_name(ctx, mod, stmt, key, value->kind, value->ref);
      }
    } else {
      /* export { name1, name2 } — re-export only listed names */
      size_t name_count = vec_get_size(exp->names);
      for (size_t i = 0; i < name_count; i++) {
        node_t name_node = (node_t)vec_get(exp->names, i);
        const char *exp_name = _get_identifier_name(name_node);
        if (!exp_name)
          continue;
        name_t dep_name = (name_t)strmap_find(dep_mod->exports, exp_name);
        if (!dep_name) {
          _report_error(ctx, name_node, "'%s' is not exported by '%s'",
                        exp_name, mod_path);
          continue;
        }
        _module_export_name(ctx, mod, stmt, exp_name, dep_name->kind,
                            dep_name->ref);
      }
    }
    break;
  }

  default:
    break;
  }
}

/* --------------------------------------------------------------------------
 *  Public API
 * -------------------------------------------------------------------------- */

void name_collector_run(context_t ctx, module_t mod) {
  if (!mod || !mod->program)
    return;

  cubec_program_node_t program = (cubec_program_node_t)mod->program;
  scope_t scope = mod->root_scope;
  bool is_static_scope = (scope->kind == SCOPE_MODULE ||
                          scope->kind == SCOPE_GLOBAL);

  size_t count = vec_get_size(program->statements);
  for (size_t i = 0; i < count; i++) {
    node_t stmt = vec_get(program->statements, i);
    _collect_statement(ctx, scope, stmt, is_static_scope);
  }
}
