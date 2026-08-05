#include "engine/name_collector.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "core/string.h"
#include "core/strmap.h"
#include "core/vec.h"
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

/** Insert a name into a scope's name table. */
static void _scope_insert_name(scope_t scope, const char *name_str,
                               enum name_kind kind, void *ref) {
  name_t name = name_create(scope->allocator, kind, ref);
  strmap_insert(scope->names, name_str, name);
}

/** Insert a name into a module's export table. */
static void _module_export_name(module_t mod, const char *name_str,
                                enum name_kind kind, void *ref) {
  name_t name = name_create(mod->allocator, kind, ref);
  strmap_insert(mod->exports, name_str, name);
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
    if (decl->declarator) {
      cubec_declaration_variable_t var =
          (cubec_declaration_variable_t)decl->declarator;
      const char *name_str = _get_identifier_name(var->identifier);
      if (name_str) {
        _scope_insert_name(scope, name_str, NAME_VARIABLE, stmt);
        if (mod && decl->is_export) {
          _module_export_name(mod, name_str, NAME_VARIABLE, stmt);
        }
      }
    }
    break;
  }

  /* func declarations */
  case CUBEC_NODE_STATEMENT_FUNCTION: {
    cubec_statement_function_t func = (cubec_statement_function_t)stmt;
    const char *name_str = _get_identifier_name(func->name);
    if (name_str) {
      _scope_insert_name(scope, name_str, NAME_FUNCTION, stmt);
      if (mod && func->is_export) {
        _module_export_name(mod, name_str, NAME_FUNCTION, stmt);
      }
    }
    break;
  }

  /* type declarations: struct, union, enum, interface, type alias, cunion */
  case CUBEC_NODE_STATEMENT_STRUCT: {
    cubec_statement_struct_t s = (cubec_statement_struct_t)stmt;
    const char *name_str = _get_identifier_name(s->name);
    if (name_str) {
      _scope_insert_name(scope, name_str, NAME_TYPE, stmt);
      if (mod && s->is_export) {
        _module_export_name(mod, name_str, NAME_TYPE, stmt);
      }
    }
    /* TODO: recurse into struct members for method/type collection */
    break;
  }
  case CUBEC_NODE_STATEMENT_UNION: {
    cubec_statement_union_t u = (cubec_statement_union_t)stmt;
    const char *name_str = _get_identifier_name(u->name);
    if (name_str) {
      _scope_insert_name(scope, name_str, NAME_TYPE, stmt);
      if (mod && u->is_export) {
        _module_export_name(mod, name_str, NAME_TYPE, stmt);
      }
    }
    /* TODO: recurse into union members for method/type collection */
    break;
  }
  case CUBEC_NODE_STATEMENT_ENUM: {
    cubec_statement_enum_t e = (cubec_statement_enum_t)stmt;
    const char *name_str = _get_identifier_name(e->name);
    if (name_str) {
      _scope_insert_name(scope, name_str, NAME_TYPE, stmt);
      if (mod && e->is_export) {
        _module_export_name(mod, name_str, NAME_TYPE, stmt);
      }
    }
    break;
  }
  case CUBEC_NODE_STATEMENT_INTERFACE: {
    cubec_statement_interface_t iface = (cubec_statement_interface_t)stmt;
    const char *name_str = _get_identifier_name(iface->name);
    if (name_str) {
      _scope_insert_name(scope, name_str, NAME_TYPE, stmt);
      if (mod && iface->is_export) {
        _module_export_name(mod, name_str, NAME_TYPE, stmt);
      }
    }
    /* TODO: recurse into interface members for method/type collection */
    break;
  }
  case CUBEC_NODE_STATEMENT_CUNION: {
    cubec_statement_cunion_t cu = (cubec_statement_cunion_t)stmt;
    const char *name_str = _get_identifier_name(cu->name);
    if (name_str) {
      _scope_insert_name(scope, name_str, NAME_TYPE, stmt);
    }
    break;
  }
  case CUBEC_NODE_STATEMENT_DECLARATION_TYPE: {
    cubec_statement_declaration_type_t t =
        (cubec_statement_declaration_type_t)stmt;
    const char *name_str = _get_identifier_name(t->name);
    if (name_str) {
      _scope_insert_name(scope, name_str, NAME_TYPE, stmt);
      if (mod && t->is_export) {
        _module_export_name(mod, name_str, NAME_TYPE, stmt);
      }
    }
    break;
  }

  /* import — load dependency module and register namespace */
  case CUBEC_NODE_STATEMENT_IMPORT: {
    cubec_statement_import_t imp = (cubec_statement_import_t)stmt;
    const char *mod_name = _get_identifier_name(imp->module_name);
    const char *mod_path = _get_string_value(imp->path);
    if (mod_name && mod_path) {
      /* Import the dependency module (read → tokenize → parse → create) */
      module_t dep_mod = context_import(ctx, mod_path);
      if (dep_mod) {
        /* Recursively run name collection on the dependency if not done */
        if (dep_mod->state == MODULE_NEW) {
          dep_mod->state = MODULE_COLLECTING;
          name_collector_run(ctx, dep_mod);
          dep_mod->state = MODULE_COLLECTED;
        }
      }
      /* Register the module as a namespace in the current scope */
      _scope_insert_name(scope, mod_name, NAME_NAMESPACE, dep_mod);
    }
    break;
  }

  /* export — re-export names from dependency module */
  case CUBEC_NODE_STATEMENT_EXPORT: {
    cubec_statement_export_t exp = (cubec_statement_export_t)stmt;
    const char *mod_path = _get_string_value(exp->path);
    if (mod_path && mod) {
      module_t dep_mod = context_import(ctx, mod_path);
      if (dep_mod) {
        if (dep_mod->state == MODULE_NEW) {
          dep_mod->state = MODULE_COLLECTING;
          name_collector_run(ctx, dep_mod);
          dep_mod->state = MODULE_COLLECTED;
        }
        /* Copy exported names from dependency into current module's exports */
        if (dep_mod->exports) {
          strmap_iter_t it = strmap_iter_first(dep_mod->exports);
          const char *key;
          while ((key = strmap_iter_next(&it)) != NULL) {
            name_t value = (name_t)strmap_find(dep_mod->exports, key);
            if (exp->is_star) {
              /* export * — re-export all */
              _module_export_name(mod, key, value->kind, value->ref);
            } else {
              /* export { name1, name2 } — re-export only listed names */
              size_t name_count = vec_get_size(exp->names);
              for (size_t i = 0; i < name_count; i++) {
                node_t name_node = (node_t)vec_get(exp->names, i);
                const char *exp_name = _get_identifier_name(name_node);
                if (exp_name && strcmp(key, exp_name) == 0) {
                  _module_export_name(mod, key, value->kind, value->ref);
                  break;
                }
              }
            }
          }
        }
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
