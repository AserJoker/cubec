#include "engine/name_collector.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "engine/diagnostic.h"
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
#include "cubec/declaration_function.h"
#include "cubec/literal_numeric.h"
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
                               value_t ref) {
  name_t existing = (name_t)strmap_find(scope->names, name_str);
  if (existing) {
    _report_error(ctx, node, "duplicate declaration of '%s'", name_str);
    return false;
  }
  name_t name = name_create(scope->allocator, kind, ref);
  strmap_insert(scope->names, name_str, name);
  return true;
}

/** Insert a reference to an existing name into a module's export table.
 *  Does NOT create a new name_t — reuses the one from scope->names. */
static void _module_export_name(context_t ctx, module_t mod, node_t node,
                                const char *name_str) {
  name_t existing = (name_t)strmap_find(mod->exports, name_str);
  if (existing) {
    _report_error(ctx, node, "duplicate export of '%s'", name_str);
    return;
  }
  name_t scope_name = (name_t)strmap_find(mod->root_scope->names, name_str);
  if (!scope_name) {
    _report_error(ctx, node, "cannot export undeclared name '%s'", name_str);
    return;
  }
  strmap_insert(mod->exports, name_str, scope_name);
}

/** Check that export is used at module scope. Returns the owning module,
 *  or NULL if not at module scope (and reports an error if is_export). */
static module_t _check_export_scope(context_t ctx, scope_t scope,
                                    node_t node, bool is_export) {
  module_t mod = (scope->kind == SCOPE_MODULE && scope->owner)
                     ? (module_t)scope->owner
                     : NULL;
  if (is_export && !mod)
    _report_error(ctx, node, "'export' can only be used at module scope");
  return mod;
}

/* --------------------------------------------------------------------------
 *  Per-statement-type collection functions
 * -------------------------------------------------------------------------- */

static void _collect_var_declaration(context_t ctx, scope_t scope,
                                     node_t stmt) {
  cubec_statement_declaration_t decl = (cubec_statement_declaration_t)stmt;
  module_t mod = _check_export_scope(ctx, scope, stmt, decl->is_export);
  if (!decl->declarator)
    return;
  cubec_declaration_variable_t var =
      (cubec_declaration_variable_t)decl->declarator;
  const char *name_str = _get_identifier_name(var->identifier);
  if (!name_str)
    return;
  /* Phase 1: insert name with ref=NULL — will be filled during definition
   * collection (phase 2) when the type/value system is rebuilt. */
  if (!_scope_insert_name(ctx, scope, stmt, name_str, NAME_VARIABLE, NULL))
    return;
  if (mod && decl->is_export)
    _module_export_name(ctx, mod, stmt, name_str);
}

static void _collect_function_declaration(context_t ctx, scope_t scope,
                                         node_t stmt) {
  cubec_statement_function_t func = (cubec_statement_function_t)stmt;
  cubec_declaration_function_t decl = (cubec_declaration_function_t)func->declarator;
  module_t mod = _check_export_scope(ctx, scope, stmt, func->is_export);
  const char *name_str = _get_identifier_name(decl->name);
  if (!name_str)
    return;
  /* Phase 1: ref=NULL — will be filled during definition collection (phase 2). */
  if (!_scope_insert_name(ctx, scope, stmt, name_str, NAME_FUNCTION, NULL))
    return;
  if (mod && func->is_export)
    _module_export_name(ctx, mod, stmt, name_str);
}

static void _collect_struct_declaration(context_t ctx, scope_t scope,
                                       node_t stmt) {
  cubec_statement_struct_t s = (cubec_statement_struct_t)stmt;
  module_t mod = _check_export_scope(ctx, scope, stmt, s->is_export);
  const char *name_str = _get_identifier_name(s->name);
  if (!name_str)
    return;
  /* Phase 1: insert type name with ref=NULL — will be filled during
   * definition collection (phase 2). */
  if (!_scope_insert_name(ctx, scope, stmt, name_str, NAME_TYPE, NULL))
    return;
  if (mod && s->is_export)
    _module_export_name(ctx, mod, stmt, name_str);
}

static void _collect_union_declaration(context_t ctx, scope_t scope,
                                      node_t stmt) {
  cubec_statement_union_t u = (cubec_statement_union_t)stmt;
  module_t mod = _check_export_scope(ctx, scope, stmt, u->is_export);
  const char *name_str = _get_identifier_name(u->name);
  if (!name_str)
    return;
  if (!_scope_insert_name(ctx, scope, stmt, name_str, NAME_TYPE, NULL))
    return;
  if (mod && u->is_export)
    _module_export_name(ctx, mod, stmt, name_str);
}

static void _collect_enum_declaration(context_t ctx, scope_t scope,
                                     node_t stmt) {
  cubec_statement_enum_t e = (cubec_statement_enum_t)stmt;
  module_t mod = _check_export_scope(ctx, scope, stmt, e->is_export);
  const char *name_str = _get_identifier_name(e->name);
  if (!name_str)
    return;
  if (!_scope_insert_name(ctx, scope, stmt, name_str, NAME_TYPE, NULL))
    return;
  if (mod && e->is_export)
    _module_export_name(ctx, mod, stmt, name_str);
}

static void _collect_interface_declaration(context_t ctx, scope_t scope,
                                          node_t stmt) {
  cubec_statement_interface_t iface = (cubec_statement_interface_t)stmt;
  module_t mod = _check_export_scope(ctx, scope, stmt, iface->is_export);
  const char *name_str = _get_identifier_name(iface->name);
  if (!name_str)
    return;
  if (!_scope_insert_name(ctx, scope, stmt, name_str, NAME_TYPE, NULL))
    return;
  if (mod && iface->is_export)
    _module_export_name(ctx, mod, stmt, name_str);
}

static void _collect_cunion_declaration(context_t ctx, scope_t scope,
                                       node_t stmt) {
  cubec_statement_cunion_t cu = (cubec_statement_cunion_t)stmt;
  const char *name_str = _get_identifier_name(cu->name);
  if (!name_str)
    return;
  _scope_insert_name(ctx, scope, stmt, name_str, NAME_TYPE, NULL);
}

static void _collect_type_alias_declaration(context_t ctx, scope_t scope,
                                           node_t stmt) {
  cubec_statement_declaration_type_t t =
      (cubec_statement_declaration_type_t)stmt;
  module_t mod = _check_export_scope(ctx, scope, stmt, t->is_export);
  const char *name_str = _get_identifier_name(t->name);
  if (!name_str)
    return;
  if (!_scope_insert_name(ctx, scope, stmt, name_str, NAME_TYPE, NULL))
    return;
  if (mod && t->is_export)
    _module_export_name(ctx, mod, stmt, name_str);
}

static void _collect_import_statement(context_t ctx, scope_t scope,
                                     node_t stmt) {
  if (scope->kind != SCOPE_MODULE) {
    _report_error(ctx, stmt, "'import' can only be used at module scope");
    return;
  }
  cubec_statement_import_t imp = (cubec_statement_import_t)stmt;
  const char *mod_name = _get_identifier_name(imp->module_name);
  const char *mod_path = _get_string_value(imp->path);
  if (!mod_name) {
    _report_error(ctx, stmt, "import statement is missing module name");
    return;
  }
  if (!mod_path) {
    _report_error(ctx, stmt, "import statement is missing module path");
    return;
  }
  module_t dep_mod = vm_import(ctx->vm, ctx, mod_path);
  if (!dep_mod) {
    _report_error(ctx, stmt, "cannot import module '%s' from '%s'",
                  mod_name, mod_path);
    return;
  }
  name_collector_run(ctx, dep_mod);
  /* Phase 1: ref=NULL — will be filled during definition collection (phase 2). */
  _scope_insert_name(ctx, scope, stmt, mod_name, NAME_NAMESPACE, NULL);
}

static void _collect_export_statement(context_t ctx, scope_t scope,
                                     node_t stmt) {
  module_t mod = (scope->kind == SCOPE_MODULE && scope->owner)
                     ? (module_t)scope->owner
                     : NULL;
  if (!mod) {
    _report_error(ctx, stmt, "'export' statement can only be used at module scope");
    return;
  }
  cubec_statement_export_t exp = (cubec_statement_export_t)stmt;
  const char *mod_path = _get_string_value(exp->path);
  if (!mod_path) {
    _report_error(ctx, stmt, "export statement is missing module path");
    return;
  }
  module_t dep_mod = vm_import(ctx->vm, ctx, mod_path);
  if (!dep_mod) {
    _report_error(ctx, stmt, "cannot export from module '%s'", mod_path);
    return;
  }
  name_collector_run(ctx, dep_mod);
  if (exp->is_star) {
    strmap_iter_t it = strmap_iter_first(dep_mod->exports);
    const char *key;
    while ((key = strmap_iter_next(&it)) != NULL) {
      name_t dep_name = (name_t)strmap_find(dep_mod->exports, key);
      strmap_insert(mod->exports, key, dep_name);
    }
  } else {
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
      strmap_insert(mod->exports, exp_name, dep_name);
    }
  }
}

/* --------------------------------------------------------------------------
 *  Statement dispatcher
 * -------------------------------------------------------------------------- */

static void _collect_statement(context_t ctx, scope_t scope, node_t stmt,
                               bool is_static_scope) {
  switch (stmt->kind) {
  case CUBEC_NODE_STATEMENT_DECLARATION:
    if (is_static_scope)
      _collect_var_declaration(ctx, scope, stmt);
    break;
  case CUBEC_NODE_STATEMENT_FUNCTION:
    _collect_function_declaration(ctx, scope, stmt);
    break;
  case CUBEC_NODE_STATEMENT_STRUCT:
    _collect_struct_declaration(ctx, scope, stmt);
    break;
  case CUBEC_NODE_STATEMENT_UNION:
    _collect_union_declaration(ctx, scope, stmt);
    break;
  case CUBEC_NODE_STATEMENT_ENUM:
    _collect_enum_declaration(ctx, scope, stmt);
    break;
  case CUBEC_NODE_STATEMENT_INTERFACE:
    _collect_interface_declaration(ctx, scope, stmt);
    break;
  case CUBEC_NODE_STATEMENT_CUNION:
    _collect_cunion_declaration(ctx, scope, stmt);
    break;
  case CUBEC_NODE_STATEMENT_DECLARATION_TYPE:
    _collect_type_alias_declaration(ctx, scope, stmt);
    break;
  case CUBEC_NODE_STATEMENT_IMPORT:
    _collect_import_statement(ctx, scope, stmt);
    break;
  case CUBEC_NODE_STATEMENT_EXPORT:
    _collect_export_statement(ctx, scope, stmt);
    break;
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
  if (mod->state >= MODULE_COLLECTED)
    return;

  mod->state = MODULE_COLLECTING;

  cubec_program_node_t program = (cubec_program_node_t)mod->program;
  scope_t scope = mod->root_scope;
  bool is_static_scope = (scope->kind == SCOPE_MODULE ||
                          scope->kind == SCOPE_GLOBAL);

  size_t count = vec_get_size(program->statements);
  for (size_t i = 0; i < count; i++) {
    node_t stmt = vec_get(program->statements, i);
    _collect_statement(ctx, scope, stmt, is_static_scope);
  }

  mod->state = MODULE_COLLECTED;
}
