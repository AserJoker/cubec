#include "engine/def_collector.h"
#include "engine/def.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "core/strmap.h"
#include "core/string.h"
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

static const char *_get_identifier_name(node_t node) {
  if (!node)
    return NULL;
  if (node->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
    cubec_literal_identifier_t id = (cubec_literal_identifier_t)node;
    return string_get(id->value);
  }
  return NULL;
}

static const char *_get_string_value(node_t node) {
  if (!node)
    return NULL;
  if (node->kind == CUBEC_NODE_LITERAL_STRING) {
    cubec_literal_string_t str = (cubec_literal_string_t)node;
    return string_get(str->value);
  }
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Def creation
 * -------------------------------------------------------------------------- */

static func_def_t _create_func_def(allocator_t allocator, node_t node) {
  func_def_t def =
      (func_def_t)allocator_alloc(allocator, sizeof(struct _func_def_t));
  def->super.allocator = allocator;
  def->super.kind = DEF_FUNC;
  def->super.node = node;
  return def;
}

static struct_def_t _create_struct_def(allocator_t allocator, node_t node) {
  struct_def_t def =
      (struct_def_t)allocator_alloc(allocator, sizeof(struct _struct_def_t));
  def->super.allocator = allocator;
  def->super.kind = DEF_STRUCT;
  def->super.node = node;
  return def;
}

static union_def_t _create_union_def(allocator_t allocator, node_t node) {
  union_def_t def =
      (union_def_t)allocator_alloc(allocator, sizeof(struct _union_def_t));
  def->super.allocator = allocator;
  def->super.kind = DEF_UNION;
  def->super.node = node;
  return def;
}

static enum_def_t _create_enum_def(allocator_t allocator, node_t node) {
  enum_def_t def =
      (enum_def_t)allocator_alloc(allocator, sizeof(struct _enum_def_t));
  def->super.allocator = allocator;
  def->super.kind = DEF_ENUM;
  def->super.node = node;
  return def;
}

static interface_def_t _create_interface_def(allocator_t allocator,
                                             node_t node) {
  interface_def_t def = (interface_def_t)allocator_alloc(
      allocator, sizeof(struct _interface_def_t));
  def->super.allocator = allocator;
  def->super.kind = DEF_INTERFACE;
  def->super.node = node;
  return def;
}

static type_alias_def_t _create_type_alias_def(allocator_t allocator,
                                               node_t node) {
  type_alias_def_t def = (type_alias_def_t)allocator_alloc(
      allocator, sizeof(struct _type_alias_def_t));
  def->super.allocator = allocator;
  def->super.kind = DEF_TYPE_ALIAS;
  def->super.node = node;
  return def;
}

static var_def_t _create_var_def(allocator_t allocator, node_t node) {
  var_def_t def =
      (var_def_t)allocator_alloc(allocator, sizeof(struct _var_def_t));
  def->super.allocator = allocator;
  def->super.kind = DEF_VAR;
  def->super.node = node;
  return def;
}

static namespace_def_t _create_namespace_def(allocator_t allocator, node_t node,
                                             module_t dep_mod) {
  namespace_def_t def = (namespace_def_t)allocator_alloc(
      allocator, sizeof(struct _namespace_def_t));
  def->super.allocator = allocator;
  def->super.kind = DEF_NAMESPACE;
  def->super.node = node;
  def->module = dep_mod;
  return def;
}

static cunion_def_t _create_cunion_def(allocator_t allocator, node_t node) {
  cunion_def_t def =
      (cunion_def_t)allocator_alloc(allocator, sizeof(struct _cunion_def_t));
  def->super.allocator = allocator;
  def->super.kind = DEF_CUNION;
  def->super.node = node;
  return def;
}

/* --------------------------------------------------------------------------
 *  Bind definition to name
 * -------------------------------------------------------------------------- */

/** Register def in scope's owned defs list and bind to name. */
static void _bind_definition(scope_t scope, const char *name_str, def_t def) {
  vec_push(scope->defs, def);
  name_t name = (name_t)strmap_find(scope->names, name_str);
  if (name)
    name->ref = def;
}

/* --------------------------------------------------------------------------
 *  Statement-level definition collection
 * -------------------------------------------------------------------------- */

static void _collect_definition(context_t ctx, scope_t scope, node_t stmt) {
  switch (stmt->kind) {
  /* var / const */
  case CUBEC_NODE_STATEMENT_DECLARATION: {
    cubec_statement_declaration_t decl = (cubec_statement_declaration_t)stmt;
    if (!decl->declarator)
      break;
    cubec_declaration_variable_t var =
        (cubec_declaration_variable_t)decl->declarator;
    const char *name_str = _get_identifier_name(var->identifier);
    if (!name_str)
      break;
    var_def_t def = _create_var_def(ctx->allocator, stmt);
    _bind_definition(scope, name_str, &def->super);
    break;
  }

  /* func */
  case CUBEC_NODE_STATEMENT_FUNCTION: {
    cubec_statement_function_t func = (cubec_statement_function_t)stmt;
    const char *name_str = _get_identifier_name(func->name);
    if (!name_str)
      break;
    func_def_t def = _create_func_def(ctx->allocator, stmt);
    _bind_definition(scope, name_str, &def->super);
    break;
  }

  /* struct */
  case CUBEC_NODE_STATEMENT_STRUCT: {
    cubec_statement_struct_t s = (cubec_statement_struct_t)stmt;
    const char *name_str = _get_identifier_name(s->name);
    if (!name_str)
      break;
    struct_def_t def = _create_struct_def(ctx->allocator, stmt);
    _bind_definition(scope, name_str, &def->super);
    break;
  }

  /* union */
  case CUBEC_NODE_STATEMENT_UNION: {
    cubec_statement_union_t u = (cubec_statement_union_t)stmt;
    const char *name_str = _get_identifier_name(u->name);
    if (!name_str)
      break;
    union_def_t def = _create_union_def(ctx->allocator, stmt);
    _bind_definition(scope, name_str, &def->super);
    break;
  }

  /* enum */
  case CUBEC_NODE_STATEMENT_ENUM: {
    cubec_statement_enum_t e = (cubec_statement_enum_t)stmt;
    const char *name_str = _get_identifier_name(e->name);
    if (!name_str)
      break;
    enum_def_t def = _create_enum_def(ctx->allocator, stmt);
    _bind_definition(scope, name_str, &def->super);
    break;
  }

  /* interface */
  case CUBEC_NODE_STATEMENT_INTERFACE: {
    cubec_statement_interface_t iface = (cubec_statement_interface_t)stmt;
    const char *name_str = _get_identifier_name(iface->name);
    if (!name_str)
      break;
    interface_def_t def = _create_interface_def(ctx->allocator, stmt);
    _bind_definition(scope, name_str, &def->super);
    break;
  }

  /* cunion */
  case CUBEC_NODE_STATEMENT_CUNION: {
    cubec_statement_cunion_t cu = (cubec_statement_cunion_t)stmt;
    const char *name_str = _get_identifier_name(cu->name);
    if (!name_str)
      break;
    cunion_def_t def = _create_cunion_def(ctx->allocator, stmt);
    _bind_definition(scope, name_str, &def->super);
    break;
  }

  /* type alias */
  case CUBEC_NODE_STATEMENT_DECLARATION_TYPE: {
    cubec_statement_declaration_type_t t =
        (cubec_statement_declaration_type_t)stmt;
    const char *name_str = _get_identifier_name(t->name);
    if (!name_str)
      break;
    type_alias_def_t def = _create_type_alias_def(ctx->allocator, stmt);
    _bind_definition(scope, name_str, &def->super);
    break;
  }

  /* import */
  case CUBEC_NODE_STATEMENT_IMPORT: {
    cubec_statement_import_t imp = (cubec_statement_import_t)stmt;
    const char *mod_name = _get_identifier_name(imp->module_name);
    const char *mod_path = _get_string_value(imp->path);
    if (!mod_name || !mod_path)
      break;
    module_t dep_mod = context_import(ctx, mod_path);
    if (!dep_mod)
      break;
    if (dep_mod->state < MODULE_RESOLVED)
      def_collector_run(ctx, dep_mod);
    namespace_def_t def = _create_namespace_def(ctx->allocator, stmt, dep_mod);
    _bind_definition(scope, mod_name, &def->super);
    break;
  }

  /* export — just ensure dependency module has been def-collected */
  case CUBEC_NODE_STATEMENT_EXPORT: {
    cubec_statement_export_t exp = (cubec_statement_export_t)stmt;
    const char *mod_path = _get_string_value(exp->path);
    if (!mod_path)
      break;
    module_t dep_mod = context_import(ctx, mod_path);
    if (!dep_mod)
      break;
    if (dep_mod->state < MODULE_RESOLVED)
      def_collector_run(ctx, dep_mod);
    break;
  }

  default:
    break;
  }
}

/* --------------------------------------------------------------------------
 *  Public API
 * -------------------------------------------------------------------------- */

void def_collector_run(context_t ctx, module_t mod) {
  if (!mod || !mod->program)
    return;
  if (mod->state < MODULE_COLLECTED)
    return;
  if (mod->state >= MODULE_RESOLVED)
    return;

  mod->state = MODULE_RESOLVING;

  cubec_program_node_t program = (cubec_program_node_t)mod->program;
  scope_t scope = mod->root_scope;

  size_t count = vec_get_size(program->statements);
  for (size_t i = 0; i < count; i++) {
    node_t stmt = vec_get(program->statements, i);
    _collect_definition(ctx, scope, stmt);
  }

  mod->state = MODULE_RESOLVED;
}
