#include "engine/checker.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
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
#include "cubec/statement_function.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/statement_import.h"
#include "cubec/statement_interface.h"
#include "cubec/declaration_variable.h"
#include "cubec/struct_field.h"
#include "cubec/enum_item.h"
#include "cubec/union_field.h"
#include "cubec/function_argument.h"
#include "cubec/interface_method.h"
#include "cubec/statement_comptime.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include "cubec/literal_char.h"
#include <string.h>

/* ===== helper: extract identifier string from AST identifier node ===== */

static const char *_ident_str(node_t id_node) {
  if (!id_node) return NULL;
  cubec_literal_identifier_t id = (cubec_literal_identifier_t)id_node;
  return string_get(id->value);
}

/* ===== builtin type registration ===== */

static semantic_type_t _register_builtin(checker_t ctx, const char *name,
                                          enum type_kind kind) {
  semantic_type_t t =
      semantic_type_create_named(ctx->allocator, name, kind);
  type_layout_compute(t, 8); /* default 64-bit */
  type_hash_ensure(t);

  /* Register in type_name_table */
  strmap_insert(ctx->type_name_table, name, t);

  /* Register in global scope */
  location_t builtin_loc = {.filename = "<builtin>",
                             .begin = {0, 0, NULL},
                             .end = {0, 0, NULL}};
  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_TYPE, builtin_loc);
  sym->type.type = t;
  sym->state = SYMBOL_EVALUATED;
  scope_push_symbol(ctx->global_scope, sym);

  return t;
}

static void _register_builtins(checker_t ctx) {
  ctx->builtin_void   = _register_builtin(ctx, "void",   TYPE_VOID);
  ctx->builtin_bool   = _register_builtin(ctx, "bool",   TYPE_BOOL);
  ctx->builtin_i8     = _register_builtin(ctx, "i8",     TYPE_I8);
  ctx->builtin_i16    = _register_builtin(ctx, "i16",    TYPE_I16);
  ctx->builtin_i32    = _register_builtin(ctx, "i32",    TYPE_I32);
  ctx->builtin_i64    = _register_builtin(ctx, "i64",    TYPE_I64);
  ctx->builtin_u8     = _register_builtin(ctx, "u8",     TYPE_U8);
  ctx->builtin_u16    = _register_builtin(ctx, "u16",    TYPE_U16);
  ctx->builtin_u32    = _register_builtin(ctx, "u32",    TYPE_U32);
  ctx->builtin_u64    = _register_builtin(ctx, "u64",    TYPE_U64);
  ctx->builtin_f16    = _register_builtin(ctx, "f16",    TYPE_F16);
  ctx->builtin_f32    = _register_builtin(ctx, "f32",    TYPE_F32);
  ctx->builtin_f64    = _register_builtin(ctx, "f64",    TYPE_F64);
  ctx->builtin_char   = _register_builtin(ctx, "char",   TYPE_CHAR);
  ctx->builtin_string = _register_builtin(ctx, "string", TYPE_STRING);
  ctx->builtin_nil    = _register_builtin(ctx, "nil",    TYPE_NIL);
  ctx->error_type     = _register_builtin(ctx, "<error>", TYPE_ERROR);
}

/* ===== checker lifecycle ===== */

static void _checker_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  checker_t ctx = (checker_t)self;
  memset(ctx, 0, sizeof(struct checker));
  ctx->allocator = allocator;

  /* Create global scope */
  location_t global_loc = {.filename = "<global>",
                            .begin = {0, 0, NULL},
                            .end = {0, 0, NULL}};
  ctx->global_scope =
      scope_create(allocator, NULL, SCOPE_GLOBAL, global_loc);
  ctx->current_scope = ctx->global_scope;

  /* Create caches */
  strmap_init_t si = {.value_auto_dispose = false};
  ctx->module_cache =
      (strmap_t)allocator_create(allocator, &g_strmap_type, &si);
  ctx->type_name_table =
      (strmap_t)allocator_create(allocator, &g_strmap_type, &si);
  ctx->type_impl_cache =
      (strmap_t)allocator_create(allocator, &g_strmap_type, &si);

  /* Init diagnostics */
  diagnostic_list_init_t dl_init = {.output = NULL};
  ctx->diagnostics = (diagnostic_list_t)allocator_create(
      allocator, &g_diagnostic_list_type, &dl_init);

  /* Init source cache */
  ctx->sources =
      (source_cache_t)allocator_create(allocator, &g_source_cache_type, NULL);

  /* Register builtin types */
  _register_builtins(ctx);
}

static void _checker_dispose(void *self, allocator_t allocator) {
  checker_t ctx = (checker_t)self;
  (void)allocator;

  /* Free builtin types (they are referenced by type_name_table but owned here) */
  allocator_free(allocator, &ctx->error_type);
  allocator_free(allocator, &ctx->builtin_nil);
  allocator_free(allocator, &ctx->builtin_string);
  allocator_free(allocator, &ctx->builtin_char);
  allocator_free(allocator, &ctx->builtin_f64);
  allocator_free(allocator, &ctx->builtin_f32);
  allocator_free(allocator, &ctx->builtin_f16);
  allocator_free(allocator, &ctx->builtin_u64);
  allocator_free(allocator, &ctx->builtin_u32);
  allocator_free(allocator, &ctx->builtin_u16);
  allocator_free(allocator, &ctx->builtin_u8);
  allocator_free(allocator, &ctx->builtin_i64);
  allocator_free(allocator, &ctx->builtin_i32);
  allocator_free(allocator, &ctx->builtin_i16);
  allocator_free(allocator, &ctx->builtin_i8);
  allocator_free(allocator, &ctx->builtin_bool);
  allocator_free(allocator, &ctx->builtin_void);

  allocator_free(allocator, &ctx->sources);
  allocator_free(allocator, &ctx->diagnostics);
  allocator_free(allocator, &ctx->type_impl_cache);
  allocator_free(allocator, &ctx->type_name_table);
  allocator_free(allocator, &ctx->module_cache);
  allocator_free(allocator, &ctx->global_scope);
}

type_t g_checker_type = {
    .size = sizeof(struct checker),
    .name = "cubec.engine.checker",
    .init = (type_init_fn_t)_checker_init,
    .dispose = (type_dispose_fn_t)_checker_dispose,
};

checker_t checker_create(allocator_t allocator) {
  return (checker_t)allocator_create(allocator, &g_checker_type, NULL);
}

void checker_dispose(checker_t ctx) {
  allocator_free(ctx->allocator, &ctx);
}

int checker_get_error_count(checker_t ctx) {
  return ctx->error_count;
}

/* ===== Pass 1: Declaration Collection ===== */

static void _collect_declarations(checker_t ctx, node_t program);

static void _collect_struct(checker_t ctx, cubec_statement_struct_t node) {
  const char *name = _ident_str(node->name);
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

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);

  strmap_insert(ctx->type_name_table, name, t);
}

static void _collect_enum(checker_t ctx, cubec_statement_enum_t node) {
  const char *name = _ident_str(node->name);
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

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);

  strmap_insert(ctx->type_name_table, name, t);
}

static void _collect_union(checker_t ctx, cubec_statement_union_t node) {
  const char *name = _ident_str(node->name);
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

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);

  strmap_insert(ctx->type_name_table, name, t);
}

static void _collect_cunion(checker_t ctx, cubec_statement_cunion_t node) {
  const char *name = _ident_str(node->name);
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

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);

  strmap_insert(ctx->type_name_table, name, t);
}

static void _collect_interface(checker_t ctx,
                                cubec_statement_interface_t node) {
  const char *name = _ident_str(node->name);
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

  struct symbol *sym =
      symbol_create(ctx->allocator, name, SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->state = SYMBOL_NAME_KNOWN;
  scope_push_symbol(ctx->global_scope, sym);

  strmap_insert(ctx->type_name_table, name, t);
}

static void _collect_function(checker_t ctx,
                               cubec_statement_function_t node) {
  const char *name = _ident_str(node->name);
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

  const char *name = _ident_str(decl->identifier);
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
  const char *name = _ident_str(node->name);
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
  const char *name = _ident_str(node->module_name);
  if (!name) return;

  const char *effective_name = node->alias ? _ident_str(node->alias) : name;

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

static void _collect_declarations(checker_t ctx, node_t program) {
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

/* ===== Pass 2: Declaration Evaluation ===== */

static void _evaluate_declarations(checker_t ctx, node_t program);

static semantic_type_t _infer_type_from_expression(checker_t ctx,
                                                     node_t expr) {
  if (!expr) return NULL;

  switch (expr->kind) {
  case CUBEC_NODE_LITERAL_NUMERIC:
    /* TODO: check suffix for specific types (i8, u16, f64, etc.) */
    return ctx->builtin_i32;
  case CUBEC_NODE_LITERAL_STRING:
    return ctx->builtin_string;
  case CUBEC_NODE_LITERAL_CHAR:
    return ctx->builtin_char;
  case CUBEC_NODE_LITERAL_IDENTIFIER: {
    const char *name = _ident_str(expr);
    struct symbol *sym = scope_lookup(ctx->current_scope, name);
    if (sym && sym->kind == SYMBOL_VARIABLE && sym->variable.type)
      return sym->variable.type;
    return NULL;
  }
  default:
    return NULL;
  }
}

static void _evaluate_struct(checker_t ctx, cubec_statement_struct_t node) {
  const char *name = _ident_str(node->name);
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
  vec_init_t vi = {.auto_dispose = false};
  t->impl->struct_type.fields =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  /* Resolve each member */
  if (node->members) {
    size_t mcount = vec_get_size(node->members);
    for (size_t i = 0; i < mcount; i++) {
      node_t member = (node_t)vec_get(node->members, i);
      if (!member) continue;

      if (member->kind == CUBEC_NODE_STRUCT_FIELD) {
        cubec_struct_field_t field = (cubec_struct_field_t)member;
        const char *fname = _ident_str(field->name);
        struct symbol *fsym = symbol_create(ctx->allocator, fname,
                                            SYMBOL_FIELD, field->super.location);
        if (field->type)
          fsym->field.type = resolver_resolve_type(ctx, field->type);
        fsym->field.index = i;
        fsym->field.is_pub = field->is_pub;
        vec_push(t->impl->struct_type.fields, fsym);
      }
      /* TODO: CUBEC_NODE_STATEMENT_DECLARATION (static fields) */
      /* TODO: CUBEC_NODE_STATEMENT_FUNCTION (methods) */
    }
  }

  type_layout_compute(t, 8);
  type_hash_ensure(t);
  sym->state = SYMBOL_EVALUATED;
}

static void _evaluate_enum(checker_t ctx, cubec_statement_enum_t node) {
  const char *name = _ident_str(node->name);
  if (!name) return;

  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_TYPE || !sym->type.type) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  semantic_type_t t = sym->type.type;

  vec_init_t vi = {.auto_dispose = false};
  t->impl->enum_type.items =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  t->impl->enum_type.backing_type = ctx->builtin_i32;

  if (node->items) {
    size_t count = vec_get_size(node->items);
    long long auto_val = 0;
    for (size_t i = 0; i < count; i++) {
      node_t item_node = (node_t)vec_get(node->items, i);
      if (!item_node || item_node->kind != CUBEC_NODE_ENUM_ITEM) continue;

      cubec_enum_item_t item = (cubec_enum_item_t)item_node;
      const char *iname = _ident_str(item->name);
      struct symbol *isym = symbol_create(ctx->allocator, iname,
                                          SYMBOL_ENUM_ITEM,
                                          item->super.location);
      isym->enum_item.owning_type = t;
      /* TODO: evaluate item->value (comptime expression) */
      isym->enum_item.value = auto_val++;
      vec_push(t->impl->enum_type.items, isym);
    }
  }

  type_layout_compute(t, 8);
  type_hash_ensure(t);
  sym->state = SYMBOL_EVALUATED;
}

static void _evaluate_union(checker_t ctx, cubec_statement_union_t node) {
  const char *name = _ident_str(node->name);
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

  vec_init_t vi = {.auto_dispose = false};
  t->impl->struct_type.fields =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  if (node->members) {
    size_t mcount = vec_get_size(node->members);
    for (size_t i = 0; i < mcount; i++) {
      node_t member = (node_t)vec_get(node->members, i);
      if (!member) continue;

      if (member->kind == CUBEC_NODE_UNION_FIELD) {
        cubec_union_field_t field = (cubec_union_field_t)member;
        const char *fname = _ident_str(field->name);
        struct symbol *fsym = symbol_create(ctx->allocator, fname,
                                            SYMBOL_FIELD, field->super.location);
        if (field->type)
          fsym->field.type = resolver_resolve_type(ctx, field->type);
        fsym->field.index = i;
        vec_push(t->impl->struct_type.fields, fsym);
      }
      /* TODO: methods, static fields, associated types, spread */
    }
  }

  type_layout_compute(t, 8);
  type_hash_ensure(t);
  sym->state = SYMBOL_EVALUATED;
}

static void _evaluate_cunion(checker_t ctx, cubec_statement_cunion_t node) {
  const char *name = _ident_str(node->name);
  if (!name) return;

  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_TYPE || !sym->type.type) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  semantic_type_t t = sym->type.type;

  vec_init_t vi = {.auto_dispose = false};
  t->impl->struct_type.fields =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  if (node->fields) {
    size_t count = vec_get_size(node->fields);
    for (size_t i = 0; i < count; i++) {
      node_t field_node = (node_t)vec_get(node->fields, i);
      if (!field_node || field_node->kind != CUBEC_NODE_STRUCT_FIELD) continue;

      cubec_struct_field_t field = (cubec_struct_field_t)field_node;
      const char *fname = _ident_str(field->name);
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

static void _evaluate_interface(checker_t ctx,
                                cubec_statement_interface_t node) {
  const char *name = _ident_str(node->name);
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

  vec_init_t vi = {.auto_dispose = false};
  t->impl->interface_type.methods =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  if (node->members) {
    size_t count = vec_get_size(node->members);
    for (size_t i = 0; i < count; i++) {
      node_t member = (node_t)vec_get(node->members, i);
      if (!member) continue;

      if (member->kind == CUBEC_NODE_INTERFACE_METHOD) {
        cubec_interface_method_t method = (cubec_interface_method_t)member;
        const char *mname = _ident_str(method->name);
        struct symbol *msym = symbol_create(ctx->allocator, mname,
                                            SYMBOL_FUNCTION,
                                            method->super.location);

        /* Resolve return type */
        semantic_type_t ret_type = method->return_type
            ? resolver_resolve_type(ctx, method->return_type)
            : ctx->builtin_void;

        /* Resolve parameters */
        vec_init_t pvi = {.auto_dispose = false};
        vec_t params =
            (vec_t)allocator_create(ctx->allocator, &g_vec_type, &pvi);
        if (method->arguments) {
          size_t acount = vec_get_size(method->arguments);
          for (size_t j = 0; j < acount; j++) {
            node_t arg = (node_t)vec_get(method->arguments, j);
            if (arg->kind == CUBEC_NODE_FUNCTION_ARGUMENT) {
              cubec_function_argument_t farg =
                  (cubec_function_argument_t)arg;
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

        msym->function.type = mtype;
        msym->state = SYMBOL_EVALUATED;
        vec_push(t->impl->interface_type.methods, msym);
      }
      /* TODO: CUBEC_NODE_STATEMENT_DECLARATION_TYPE (associated types) */
    }
  }

  type_layout_compute(t, 8);
  type_hash_ensure(t);
  sym->state = SYMBOL_EVALUATED;
}

static void _evaluate_function(checker_t ctx,
                               cubec_statement_function_t node) {
  const char *name = _ident_str(node->name);
  if (!name) return;

  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_FUNCTION) return;
  if (sym->state == SYMBOL_EVALUATED) return;

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

  sym->function.type = ftype;
  sym->function.is_comptime = node->is_comptime;
  /* Body NOT checked in Pass 2 — deferred to Pass 3 */
  sym->state = SYMBOL_EVALUATED;
}

static void _evaluate_variable(checker_t ctx,
                               cubec_statement_declaration_t node) {
  cubec_declaration_variable_t decl =
      (cubec_declaration_variable_t)node->declarator;
  if (!decl) return;

  const char *name = _ident_str(decl->identifier);
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
    var_type = _infer_type_from_expression(ctx, decl->expression);

  /* extern/builtin require explicit type */
  if (node->is_extern || node->is_builtin) {
    if (!decl->type) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           node->super.location,
                           "extern/builtin variable '%s' requires type annotation",
                           name);
      ctx->error_count++;
      var_type = ctx->error_type;
    }
  }

  if (!var_type) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "cannot infer type for variable '%s'", name);
    ctx->error_count++;
    var_type = ctx->error_type;
  }

  /* Check type completeness — void is never a valid variable type */
  if (var_type && var_type->impl->kind == TYPE_VOID) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "variable '%s' has incomplete type 'void'", name);
    ctx->error_count++;
  } else if (var_type && var_type->is_incomplete &&
             var_type->impl->kind != TYPE_ERROR) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "variable '%s' has incomplete type '%s'",
                         name, var_type->name ? var_type->name : "<anonymous>");
    ctx->error_count++;
  }

  sym->variable.type = var_type;
  sym->variable.is_comptime = node->is_comptime;
  sym->variable.is_mutable = true;
  sym->state = SYMBOL_EVALUATED;
}

static void _evaluate_type_alias(checker_t ctx,
                                 cubec_statement_declaration_type_t node) {
  const char *name = _ident_str(node->name);
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
  const char *name = _ident_str(node->module_name);
  if (!name) return;

  const char *effective_name = node->alias ? _ident_str(node->alias) : name;
  struct symbol *sym = scope_lookup_local(ctx->global_scope, effective_name);
  if (!sym || sym->kind != SYMBOL_MODULE) return;
  if (sym->state == SYMBOL_EVALUATED) return;

  /* TODO: module resolution — load and check imported module */

  sym->state = SYMBOL_EVALUATED;
}

static void _evaluate_comptime_block(checker_t ctx,
                                     cubec_statement_comptime_block_t node) {
  /* TODO: comptime evaluator — execute block at compile time */
  (void)ctx;
  (void)node;
}

static void _evaluate_comptime_if(checker_t ctx,
                                  cubec_statement_comptime_if_t node) {
  /* TODO: evaluate condition at compile time, process branches */
  (void)ctx;
  (void)node;
}

static void _evaluate_comptime_for(checker_t ctx,
                                   cubec_statement_comptime_for_t node) {
  /* TODO: unroll loop at compile time, process body declarations */
  (void)ctx;
  (void)node;
}

static void _evaluate_declarations(checker_t ctx, node_t program) {
  cubec_program_node_t prog = (cubec_program_node_t)program;
  if (!prog || !prog->statements) return;

  size_t count = vec_get_size(prog->statements);
  for (size_t i = 0; i < count; i++) {
    node_t stmt = (node_t)vec_get(prog->statements, i);
    if (!stmt) continue;

    switch (stmt->kind) {
    case CUBEC_NODE_STATEMENT_STRUCT:
      _evaluate_struct(ctx, (cubec_statement_struct_t)stmt);
      break;
    case CUBEC_NODE_STATEMENT_ENUM:
      _evaluate_enum(ctx, (cubec_statement_enum_t)stmt);
      break;
    case CUBEC_NODE_STATEMENT_UNION:
      _evaluate_union(ctx, (cubec_statement_union_t)stmt);
      break;
    case CUBEC_NODE_STATEMENT_CUNION:
      _evaluate_cunion(ctx, (cubec_statement_cunion_t)stmt);
      break;
    case CUBEC_NODE_STATEMENT_INTERFACE:
      _evaluate_interface(ctx, (cubec_statement_interface_t)stmt);
      break;
    case CUBEC_NODE_STATEMENT_FUNCTION:
      _evaluate_function(ctx, (cubec_statement_function_t)stmt);
      break;
    case CUBEC_NODE_STATEMENT_DECLARATION:
      _evaluate_variable(ctx, (cubec_statement_declaration_t)stmt);
      break;
    case CUBEC_NODE_STATEMENT_DECLARATION_TYPE:
      _evaluate_type_alias(ctx, (cubec_statement_declaration_type_t)stmt);
      break;
    case CUBEC_NODE_STATEMENT_IMPORT:
      _evaluate_import(ctx, (cubec_statement_import_t)stmt);
      break;
    case CUBEC_NODE_STATEMENT_COMPTIME_BLOCK:
      _evaluate_comptime_block(ctx,
                               (cubec_statement_comptime_block_t)stmt);
      break;
    case CUBEC_NODE_STATEMENT_COMPTIME_IF:
      _evaluate_comptime_if(ctx, (cubec_statement_comptime_if_t)stmt);
      break;
    case CUBEC_NODE_STATEMENT_COMPTIME_FOR:
      _evaluate_comptime_for(ctx, (cubec_statement_comptime_for_t)stmt);
      break;
    default:
      break;
    }
  }
}

/* ===== main entry ===== */

void checker_check_program(checker_t ctx, node_t program) {
  if (!ctx || !program) return;

  /* Pass 1: Declaration collection */
  _collect_declarations(ctx, program);

  /* Pass 2: Sequential evaluation and checking */
  _evaluate_declarations(ctx, program);

  /* Pass 3: Function body checking — TODO */
  /* Pass 4: Generic instantiation — TODO */
}
