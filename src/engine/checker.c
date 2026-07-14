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
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include "cubec/literal_char.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_enum.h"
#include "cubec/statement_union.h"
#include "cubec/statement_cunion.h"
#include "cubec/statement_function.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/statement_import.h"
#include "cubec/statement_interface.h"
#include "cubec/statement_block.h"
#include "cubec/statement_expression.h"
#include "cubec/statement_return.h"
#include "cubec/statement_if.h"
#include "cubec/statement_while.h"
#include "cubec/statement_do_while.h"
#include "cubec/statement_for.h"
#include "cubec/statement_foreach.h"
#include "cubec/statement_break.h"
#include "cubec/statement_continue.h"
#include "cubec/statement_defer.h"
#include "cubec/statement_switch.h"
#include "cubec/statement_empty.h"
#include "cubec/statement_comptime.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_postfix_unary.h"
#include "cubec/expression_call.h"
#include "cubec/expression_member.h"
#include "cubec/expression_ternary.h"
#include "cubec/expression_group.h"
#include "cubec/expression_sizeof.h"
#include "cubec/expression_alignof.h"
#include "cubec/expression_typeof.h"
#include "cubec/expression_slice.h"
#include "cubec/expression_function.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/expression_initialize_field.h"
#include "cubec/expression_comma.h"
#include "cubec/expression_spread.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/switch_match.h"
#include "cubec/function_capture.h"
#include "cubec/declaration_variable.h"
#include "cubec/struct_field.h"
#include "cubec/enum_item.h"
#include "cubec/union_field.h"
#include "cubec/function_argument.h"
#include "cubec/interface_method.h"
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
  case CUBEC_NODE_LITERAL_NUMERIC: {
    cubec_literal_numeric_t num = (cubec_literal_numeric_t)expr;
    switch (num->numeric_type) {
    case CUBEC_LITERAL_NUMERIC_TYPE_I8:  return ctx->builtin_i8;
    case CUBEC_LITERAL_NUMERIC_TYPE_I16: return ctx->builtin_i16;
    case CUBEC_LITERAL_NUMERIC_TYPE_I32: return ctx->builtin_i32;
    case CUBEC_LITERAL_NUMERIC_TYPE_I64: return ctx->builtin_i64;
    case CUBEC_LITERAL_NUMERIC_TYPE_U8:  return ctx->builtin_u8;
    case CUBEC_LITERAL_NUMERIC_TYPE_U16: return ctx->builtin_u16;
    case CUBEC_LITERAL_NUMERIC_TYPE_U32: return ctx->builtin_u32;
    case CUBEC_LITERAL_NUMERIC_TYPE_U64: return ctx->builtin_u64;
    case CUBEC_LITERAL_NUMERIC_TYPE_F16: return ctx->builtin_f16;
    case CUBEC_LITERAL_NUMERIC_TYPE_F32: return ctx->builtin_f32;
    case CUBEC_LITERAL_NUMERIC_TYPE_F64: return ctx->builtin_f64;
    default:
      /* DEFAULT integer → i32, DEFAULT float → f64 */
      return num->kind == CUBEC_LITERAL_NUMERIC_KIND_FLOAT
                 ? ctx->builtin_f64
                 : ctx->builtin_i32;
    }
  }
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
      } else if (member->kind == CUBEC_NODE_STATEMENT_FUNCTION) {
        cubec_statement_function_t mfn = (cubec_statement_function_t)member;
        const char *mname = _ident_str(mfn->name);
        struct symbol *msym = symbol_create(ctx->allocator, mname,
                                            SYMBOL_FUNCTION, mfn->super.location);
        semantic_type_t ret_type = mfn->return_type
            ? resolver_resolve_type(ctx, mfn->return_type)
            : ctx->builtin_void;
        vec_init_t pvi = {.auto_dispose = false};
        vec_t params =
            (vec_t)allocator_create(ctx->allocator, &g_vec_type, &pvi);
        if (mfn->arguments) {
          size_t acount = vec_get_size(mfn->arguments);
          for (size_t j = 0; j < acount; j++) {
            node_t arg = (node_t)vec_get(mfn->arguments, j);
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
            ctx->allocator, ret_type, params, mfn->is_c_variadic);
        type_hash_ensure(mtype);
        msym->function.type = mtype;
        msym->function.is_comptime = mfn->is_comptime;
        msym->state = SYMBOL_NAME_KNOWN; /* body checked in Pass 3 */
        vec_push(t->instance_methods, msym);
      } else if (member->kind == CUBEC_NODE_STATEMENT_DECLARATION) {
        cubec_statement_declaration_t sdecl =
            (cubec_statement_declaration_t)member;
        cubec_declaration_variable_t vdecl =
            (cubec_declaration_variable_t)sdecl->declarator;
        if (vdecl) {
          const char *vname = _ident_str(vdecl->identifier);
          struct symbol *vsym = symbol_create(ctx->allocator, vname,
                                              SYMBOL_VARIABLE,
                                              sdecl->super.location);
          if (vdecl->type)
            vsym->variable.type = resolver_resolve_type(ctx, vdecl->type);
          vsym->variable.is_comptime = sdecl->is_comptime;
          vsym->variable.is_mutable = true;
          vsym->state = SYMBOL_NAME_KNOWN; /* initializer in Pass 3 */
          vec_push(t->static_fields, vsym);
        }
      }
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
      } else if (member->kind == CUBEC_NODE_STATEMENT_FUNCTION) {
        cubec_statement_function_t mfn = (cubec_statement_function_t)member;
        const char *mname = _ident_str(mfn->name);
        struct symbol *msym = symbol_create(ctx->allocator, mname,
                                            SYMBOL_FUNCTION, mfn->super.location);
        semantic_type_t ret_type = mfn->return_type
            ? resolver_resolve_type(ctx, mfn->return_type)
            : ctx->builtin_void;
        vec_init_t pvi = {.auto_dispose = false};
        vec_t params =
            (vec_t)allocator_create(ctx->allocator, &g_vec_type, &pvi);
        if (mfn->arguments) {
          size_t acount = vec_get_size(mfn->arguments);
          for (size_t j = 0; j < acount; j++) {
            node_t arg = (node_t)vec_get(mfn->arguments, j);
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
            ctx->allocator, ret_type, params, mfn->is_c_variadic);
        type_hash_ensure(mtype);
        msym->function.type = mtype;
        msym->function.is_comptime = mfn->is_comptime;
        msym->state = SYMBOL_NAME_KNOWN;
        vec_push(t->instance_methods, msym);
      } else if (member->kind == CUBEC_NODE_STATEMENT_DECLARATION) {
        cubec_statement_declaration_t sdecl =
            (cubec_statement_declaration_t)member;
        cubec_declaration_variable_t vdecl =
            (cubec_declaration_variable_t)sdecl->declarator;
        if (vdecl) {
          const char *vname = _ident_str(vdecl->identifier);
          struct symbol *vsym = symbol_create(ctx->allocator, vname,
                                              SYMBOL_VARIABLE,
                                              sdecl->super.location);
          if (vdecl->type)
            vsym->variable.type = resolver_resolve_type(ctx, vdecl->type);
          vsym->variable.is_comptime = sdecl->is_comptime;
          vsym->variable.is_mutable = true;
          vsym->state = SYMBOL_NAME_KNOWN;
          vec_push(t->static_fields, vsym);
        }
      }
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
      } else if (member->kind == CUBEC_NODE_STATEMENT_DECLARATION_TYPE) {
        cubec_statement_declaration_type_t tdecl =
            (cubec_statement_declaration_type_t)member;
        const char *tname = _ident_str(tdecl->name);
        struct symbol *tsym = symbol_create(ctx->allocator, tname,
                                            SYMBOL_TYPE, tdecl->super.location);
        if (tdecl->type_value) {
          tsym->type.type = resolver_resolve_type(ctx, tdecl->type_value);
        }
        /* Generic associated type: skip resolution for now */
        tsym->state = SYMBOL_NAME_KNOWN;
        vec_push(t->associated_types, tsym);
      }
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

/* ===== Pass 3: Function Body Checking ===== */

/* --- type utility functions --- */

static bool _is_numeric_type(semantic_type_t t) {
  if (!t || !t->impl) return false;
  enum type_kind k = t->impl->kind;
  return (k >= TYPE_I8 && k <= TYPE_U64) || (k >= TYPE_F16 && k <= TYPE_F64);
}

static bool _is_integer_type(semantic_type_t t) {
  if (!t || !t->impl) return false;
  enum type_kind k = t->impl->kind;
  return k >= TYPE_I8 && k <= TYPE_U64;
}

static bool _is_bool_type(semantic_type_t t) {
  return t && t->impl && t->impl->kind == TYPE_BOOL;
}

static semantic_type_t _common_type(checker_t ctx,
                                     semantic_type_t a,
                                     semantic_type_t b) {
  if (!a || !b) return ctx->error_type;
  if (semantic_type_equals(a, b)) return a;
  /* int + float → float */
  if (_is_integer_type(a) && b->impl->kind >= TYPE_F16 &&
      b->impl->kind <= TYPE_F64)
    return b;
  if (_is_integer_type(b) && a->impl->kind >= TYPE_F16 &&
      a->impl->kind <= TYPE_F64)
    return a;
  /* float widening */
  if (a->impl->size >= b->impl->size) return a;
  return b;
}

static bool _is_lvalue(node_t expr) {
  if (!expr) return false;
  switch (expr->kind) {
  case CUBEC_NODE_LITERAL_IDENTIFIER:
  case CUBEC_NODE_EXPRESSION_MEMBER:
  case CUBEC_NODE_EXPRESSION_DEREF:
    return true;
  default:
    return false;
  }
}

/* --- expression type checker --- */

static semantic_type_t _check_expression(checker_t ctx, node_t expr);

static semantic_type_t _check_literal_numeric(checker_t ctx,
                                              cubec_literal_numeric_t num) {
  switch (num->numeric_type) {
  case CUBEC_LITERAL_NUMERIC_TYPE_I8:  return ctx->builtin_i8;
  case CUBEC_LITERAL_NUMERIC_TYPE_I16: return ctx->builtin_i16;
  case CUBEC_LITERAL_NUMERIC_TYPE_I32: return ctx->builtin_i32;
  case CUBEC_LITERAL_NUMERIC_TYPE_I64: return ctx->builtin_i64;
  case CUBEC_LITERAL_NUMERIC_TYPE_U8:  return ctx->builtin_u8;
  case CUBEC_LITERAL_NUMERIC_TYPE_U16: return ctx->builtin_u16;
  case CUBEC_LITERAL_NUMERIC_TYPE_U32: return ctx->builtin_u32;
  case CUBEC_LITERAL_NUMERIC_TYPE_U64: return ctx->builtin_u64;
  case CUBEC_LITERAL_NUMERIC_TYPE_F16: return ctx->builtin_f16;
  case CUBEC_LITERAL_NUMERIC_TYPE_F32: return ctx->builtin_f32;
  case CUBEC_LITERAL_NUMERIC_TYPE_F64: return ctx->builtin_f64;
  default:
    return num->kind == CUBEC_LITERAL_NUMERIC_KIND_FLOAT
               ? ctx->builtin_f64
               : ctx->builtin_i32;
  }
}

static semantic_type_t _check_expression(checker_t ctx, node_t expr) {
  if (!expr) return ctx->error_type;

  switch (expr->kind) {

  /* --- literals --- */
  case CUBEC_NODE_LITERAL_NUMERIC:
    return _check_literal_numeric(ctx, (cubec_literal_numeric_t)expr);

  case CUBEC_NODE_LITERAL_STRING:
    return ctx->builtin_string;

  case CUBEC_NODE_LITERAL_CHAR:
    return ctx->builtin_char;

  case CUBEC_NODE_LITERAL_IDENTIFIER: {
    const char *name = _ident_str(expr);
    if (!name) return ctx->error_type;
    struct symbol *sym = scope_lookup(ctx->current_scope, name);
    if (!sym) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                           "undeclared identifier '%s'", name);
      ctx->error_count++;
      return ctx->error_type;
    }
    switch (sym->kind) {
    case SYMBOL_VARIABLE: return sym->variable.type;
    case SYMBOL_FUNCTION: return sym->function.type;
    case SYMBOL_ENUM_ITEM: return sym->enum_item.owning_type;
    case SYMBOL_GENERIC_PARAM: return ctx->error_type; /* TODO */
    default: return ctx->error_type;
    }
  }

  /* --- binary operators --- */
  case CUBEC_NODE_EXPRESSION_BINARY: {
    cubec_expression_binary_t bin = (cubec_expression_binary_t)expr;
    const char *op = string_get(bin->opt);

    /* prefix unary: left is NULL */
    if (!bin->left) {
      semantic_type_t rt = _check_expression(ctx, bin->right);
      if (rt->impl->kind == TYPE_ERROR) return ctx->error_type;

      if (strcmp(op, "!") == 0) {
        if (!_is_bool_type(rt)) {
          diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                               expr->location,
                               "operator '!' requires bool operand");
          ctx->error_count++;
        }
        return ctx->builtin_bool;
      }
      if (strcmp(op, "-") == 0) {
        if (!_is_numeric_type(rt)) {
          diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                               expr->location,
                               "operator '-' requires numeric operand");
          ctx->error_count++;
        }
        return rt;
      }
      if (strcmp(op, "~") == 0) {
        if (!_is_integer_type(rt)) {
          diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                               expr->location,
                               "operator '~' requires integer operand");
          ctx->error_count++;
        }
        return rt;
      }
      /* unknown prefix op */
      return rt;
    }

    /* binary operators */
    semantic_type_t lt = _check_expression(ctx, bin->left);
    semantic_type_t rt = _check_expression(ctx, bin->right);
    if (lt->impl->kind == TYPE_ERROR || rt->impl->kind == TYPE_ERROR)
      return ctx->error_type;

    /* arithmetic: + - * / % */
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
        strcmp(op, "*") == 0 || strcmp(op, "/") == 0 ||
        strcmp(op, "%") == 0) {
      if (!_is_numeric_type(lt) || !_is_numeric_type(rt)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             expr->location,
                             "arithmetic operator '%s' requires numeric operands",
                             op);
        ctx->error_count++;
        return ctx->error_type;
      }
      return _common_type(ctx, lt, rt);
    }

    /* comparison: == != < > <= >= */
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
        strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
        strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0) {
      return ctx->builtin_bool;
    }

    /* logical: && || */
    if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0) {
      if (!_is_bool_type(lt) || !_is_bool_type(rt)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             expr->location,
                             "logical operator '%s' requires bool operands",
                             op);
        ctx->error_count++;
      }
      return ctx->builtin_bool;
    }

    /* bitwise: & | ^ << >> */
    if (strcmp(op, "&") == 0 || strcmp(op, "|") == 0 ||
        strcmp(op, "^") == 0 || strcmp(op, "<<") == 0 ||
        strcmp(op, ">>") == 0) {
      if (!_is_integer_type(lt) || !_is_integer_type(rt)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             expr->location,
                             "bitwise operator '%s' requires integer operands",
                             op);
        ctx->error_count++;
        return ctx->error_type;
      }
      return _common_type(ctx, lt, rt);
    }

    /* fallback */
    return ctx->error_type;
  }

  /* --- assignment --- */
  case CUBEC_NODE_EXPRESSION_ASSIGNMENT: {
    cubec_expression_assignment_t asgn = (cubec_expression_assignment_t)expr;
    const char *op = string_get(asgn->opt);

    if (!_is_lvalue(asgn->left)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                           "left side of assignment is not a valid lvalue");
      ctx->error_count++;
    }

    semantic_type_t lt = _check_expression(ctx, asgn->left);
    semantic_type_t rt = _check_expression(ctx, asgn->right);
    if (lt->impl->kind == TYPE_ERROR || rt->impl->kind == TYPE_ERROR)
      return ctx->error_type;

    /* simple assignment = */
    if (strcmp(op, "=") == 0) {
      if (!semantic_type_can_implicit_convert(rt, lt)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             expr->location,
                             "cannot assign '%s' to '%s'",
                             rt->name ? rt->name : "<anonymous>",
                             lt->name ? lt->name : "<anonymous>");
        ctx->error_count++;
      }
      return lt;
    }

    /* compound assignment: += -= etc. */
    if (!semantic_type_can_implicit_convert(rt, lt)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                           "type mismatch in compound assignment");
      ctx->error_count++;
    }
    return lt;
  }

  /* --- function call --- */
  case CUBEC_NODE_EXPRESSION_CALL: {
    cubec_expression_call_t call = (cubec_expression_call_t)expr;
    semantic_type_t callee_type = _check_expression(ctx, call->callee);
    if (callee_type->impl->kind == TYPE_ERROR) return ctx->error_type;

    if (callee_type->impl->kind != TYPE_FUNCTION) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                           "call of non-function type");
      ctx->error_count++;
      return ctx->error_type;
    }

    /* check arguments */
    vec_t params = callee_type->impl->function.params;
    size_t param_count = params ? vec_get_size(params) : 0;
    size_t arg_count = call->arguments ? vec_get_size(call->arguments) : 0;
    bool is_variadic = callee_type->impl->function.is_variadic;

    if (!is_variadic && arg_count != param_count) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                           "expected %zu arguments, got %zu",
                           param_count, arg_count);
      ctx->error_count++;
    }

    for (size_t i = 0; i < arg_count; i++) {
      node_t arg = (node_t)vec_get(call->arguments, i);
      semantic_type_t at = _check_expression(ctx, arg);
      if (i < param_count && at->impl->kind != TYPE_ERROR) {
        semantic_type_t pt = (semantic_type_t)vec_get(params, i);
        if (!semantic_type_can_implicit_convert(at, pt)) {
          diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                               arg->location,
                               "argument %zu: cannot convert '%s' to '%s'",
                               i + 1,
                               at->name ? at->name : "<anonymous>",
                               pt->name ? pt->name : "<anonymous>");
          ctx->error_count++;
        }
      }
    }

    return callee_type->impl->function.return_type;
  }

  /* --- member access (.) --- */
  case CUBEC_NODE_EXPRESSION_MEMBER: {
    cubec_expression_member_t mem = (cubec_expression_member_t)expr;
    semantic_type_t host_type = _check_expression(ctx, mem->host);
    if (host_type->impl->kind == TYPE_ERROR) return ctx->error_type;

    const char *fname = _ident_str((node_t)mem->field);
    if (!fname) return ctx->error_type;

    /* search fields */
    if (host_type->impl->kind == TYPE_STRUCT ||
        host_type->impl->kind == TYPE_UNION ||
        host_type->impl->kind == TYPE_CUNION) {
      vec_t fields = host_type->impl->struct_type.fields;
      size_t fcount = fields ? vec_get_size(fields) : 0;
      for (size_t i = 0; i < fcount; i++) {
        struct symbol *f = (struct symbol *)vec_get(fields, i);
        if (f && f->name && strcmp(f->name, fname) == 0)
          return f->field.type;
      }
      /* search instance methods */
      size_t mcount = vec_get_size(host_type->instance_methods);
      for (size_t i = 0; i < mcount; i++) {
        struct symbol *m = (struct symbol *)vec_get(host_type->instance_methods, i);
        if (m && m->name && strcmp(m->name, fname) == 0)
          return m->function.type;
      }
    }

    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "type '%s' has no member '%s'",
                         host_type->name ? host_type->name : "<anonymous>",
                         fname);
    ctx->error_count++;
    return ctx->error_type;
  }

  /* --- namespace access (::) --- */
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS: {
    cubec_expression_namespace_access_t ns =
        (cubec_expression_namespace_access_t)expr;
    semantic_type_t host_type = _check_expression(ctx, ns->host);
    if (host_type->impl->kind == TYPE_ERROR) return ctx->error_type;

    const char *fname = _ident_str((node_t)ns->field);
    if (!fname) return ctx->error_type;

    /* search static members */
    size_t sfcount = vec_get_size(host_type->static_fields);
    for (size_t i = 0; i < sfcount; i++) {
      struct symbol *sf = (struct symbol *)vec_get(host_type->static_fields, i);
      if (sf && sf->name && strcmp(sf->name, fname) == 0)
        return sf->variable.type;
    }
    size_t smcount = vec_get_size(host_type->static_methods);
    for (size_t i = 0; i < smcount; i++) {
      struct symbol *sm = (struct symbol *)vec_get(host_type->static_methods, i);
      if (sm && sm->name && strcmp(sm->name, fname) == 0)
        return sm->function.type;
    }
    size_t atcount = vec_get_size(host_type->associated_types);
    for (size_t i = 0; i < atcount; i++) {
      struct symbol *at = (struct symbol *)vec_get(host_type->associated_types, i);
      if (at && at->name && strcmp(at->name, fname) == 0)
        return at->type.type;
    }

    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "type '%s' has no static member '%s'",
                         host_type->name ? host_type->name : "<anonymous>",
                         fname);
    ctx->error_count++;
    return ctx->error_type;
  }

  /* --- deref (.*) --- */
  case CUBEC_NODE_EXPRESSION_DEREF: {
    cubec_expression_postfix_unary_t pf =
        (cubec_expression_postfix_unary_t)expr;
    semantic_type_t host_type = _check_expression(ctx, pf->left);
    if (host_type->impl->kind == TYPE_ERROR) return ctx->error_type;

    if (host_type->impl->kind != TYPE_POINTER) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                           "cannot dereference non-pointer type");
      ctx->error_count++;
      return ctx->error_type;
    }
    return host_type->impl->pointer.pointee;
  }

  /* --- address (.&) --- */
  case CUBEC_NODE_EXPRESSION_ADDR: {
    cubec_expression_postfix_unary_t pf =
        (cubec_expression_postfix_unary_t)expr;
    semantic_type_t host_type = _check_expression(ctx, pf->left);
    if (host_type->impl->kind == TYPE_ERROR) return ctx->error_type;

    if (!_is_lvalue(pf->left)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                           "cannot take address of non-lvalue");
      ctx->error_count++;
    }
    return semantic_type_create_pointer(ctx->allocator, host_type);
  }

  /* --- try (.?) --- */
  case CUBEC_NODE_EXPRESSION_TRY: {
    cubec_expression_postfix_unary_t pf =
        (cubec_expression_postfix_unary_t)expr;
    /* TODO: unwrap/try semantics */
    return _check_expression(ctx, pf->left);
  }

  /* --- ternary --- */
  case CUBEC_NODE_EXPRESSION_TERNARY: {
    cubec_expression_ternary_t tern = (cubec_expression_ternary_t)expr;
    semantic_type_t ct = _check_expression(ctx, tern->condition);
    semantic_type_t tt = _check_expression(ctx, tern->consequent);
    semantic_type_t ft = _check_expression(ctx, tern->alternate);

    if (!_is_bool_type(ct)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                           "ternary condition must be bool");
      ctx->error_count++;
    }
    return _common_type(ctx, tt, ft);
  }

  /* --- group --- */
  case CUBEC_NODE_EXPRESSION_GROUP: {
    cubec_expression_group_t grp = (cubec_expression_group_t)expr;
    return _check_expression(ctx, grp->inner);
  }

  /* --- sizeof --- */
  case CUBEC_NODE_EXPRESSION_SIZEOF: {
    cubec_expression_sizeof_t sz = (cubec_expression_sizeof_t)expr;
    /* Try as type expression first, then as value expression */
    semantic_type_t t = resolver_resolve_type(ctx, sz->expression);
    if (t->impl->kind == TYPE_ERROR) {
      t = _check_expression(ctx, sz->expression);
    }
    (void)t;
    return ctx->builtin_u64;
  }

  /* --- alignof --- */
  case CUBEC_NODE_EXPRESSION_ALIGNOF: {
    cubec_expression_alignof_t al = (cubec_expression_alignof_t)expr;
    semantic_type_t t = resolver_resolve_type(ctx, al->expression);
    if (t->impl->kind == TYPE_ERROR) {
      t = _check_expression(ctx, al->expression);
    }
    (void)t;
    return ctx->builtin_u64;
  }

  /* --- typeof --- */
  case CUBEC_NODE_EXPRESSION_TYPEOF: {
    cubec_expression_typeof_t to = (cubec_expression_typeof_t)expr;
    semantic_type_t inner = _check_expression(ctx, to->expression);
    semantic_type_t t =
        semantic_type_create_named(ctx->allocator, NULL, TYPE_TYPE);
    t->impl->type_of.inner = inner;
    t->is_incomplete = false;
    return t;
  }

  /* --- slice expression --- */
  case CUBEC_NODE_EXPRESSION_SLICE: {
    cubec_expression_slice_t sl = (cubec_expression_slice_t)expr;
    semantic_type_t ht = _check_expression(ctx, sl->host);
    if (ht->impl->kind == TYPE_ERROR) return ctx->error_type;

    if (sl->start) _check_expression(ctx, sl->start);
    if (sl->length) _check_expression(ctx, sl->length);

    /* array/slice -> slice of element type */
    if (ht->impl->kind == TYPE_ARRAY)
      return semantic_type_create_slice(ctx->allocator,
                                         ht->impl->array.element);
    if (ht->impl->kind == TYPE_SLICE)
      return ht;

    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                         "cannot slice type '%s'",
                         ht->name ? ht->name : "<anonymous>");
    ctx->error_count++;
    return ctx->error_type;
  }

  /* --- anonymous function --- */
  case CUBEC_NODE_EXPRESSION_FUNCTION: {
    cubec_expression_function_t fn = (cubec_expression_function_t)expr;
    /* TODO: full scope setup for anonymous function */
    (void)fn;
    return ctx->error_type;
  }

  /* --- initialize list --- */
  case CUBEC_NODE_EXPRESSION_INITIALIZE_LIST: {
    cubec_expression_initialize_list_t il =
        (cubec_expression_initialize_list_t)expr;
    if (il->type) {
      semantic_type_t t = resolver_resolve_type(ctx, il->type);
      /* TODO: check items match fields */
      return t;
    }
    /* TODO: anonymous init list type inference */
    return ctx->error_type;
  }

  /* --- comma --- */
  case CUBEC_NODE_EXPRESSION_COMMA: {
    cubec_expression_comma_t cm = (cubec_expression_comma_t)expr;
    _check_expression(ctx, cm->left);
    return _check_expression(ctx, cm->right);
  }

  /* --- spread --- */
  case CUBEC_NODE_EXPRESSION_SPREAD: {
    cubec_expression_spread_t sp = (cubec_expression_spread_t)expr;
    return _check_expression(ctx, sp->value);
  }

  /* --- generic instantiation --- */
  case CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION: {
    cubec_expression_generic_instantiation_t gi =
        (cubec_expression_generic_instantiation_t)expr;
    /* TODO: generic instantiation */
    return _check_expression(ctx, gi->callee);
  }

  /* --- type expressions used as values --- */
  case CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER:
  case CUBEC_NODE_EXPRESSION_TYPE_STRUCT:
  case CUBEC_NODE_EXPRESSION_TYPE_ENUM:
  case CUBEC_NODE_EXPRESSION_TYPE_UNION:
  case CUBEC_NODE_EXPRESSION_TYPE_INTERFACE:
  case CUBEC_NODE_EXPRESSION_TYPE_FUNCTION:
    return resolver_resolve_type(ctx, expr);

  default:
    return ctx->error_type;
  }
}

/* --- statement checker --- */

static void _check_statement(checker_t ctx, node_t stmt,
                              semantic_type_t return_type);

static void _check_block(checker_t ctx, cubec_statement_block_t block,
                          semantic_type_t return_type) {
  if (!block || !block->statements) return;
  scope_t saved = ctx->current_scope;
  ctx->current_scope = scope_create(ctx->allocator, ctx->current_scope,
                                     SCOPE_BLOCK, block->super.location);
  size_t count = vec_get_size(block->statements);
  for (size_t i = 0; i < count; i++) {
    node_t s = (node_t)vec_get(block->statements, i);
    _check_statement(ctx, s, return_type);
  }
  ctx->current_scope = saved;
}

static void _check_statement(checker_t ctx, node_t stmt,
                              semantic_type_t return_type) {
  if (!stmt) return;

  switch (stmt->kind) {

  case CUBEC_NODE_STATEMENT_BLOCK:
    _check_block(ctx, (cubec_statement_block_t)stmt, return_type);
    break;

  case CUBEC_NODE_STATEMENT_EXPRESSION: {
    cubec_statement_expression_t se = (cubec_statement_expression_t)stmt;
    _check_expression(ctx, se->expression);
    break;
  }

  case CUBEC_NODE_STATEMENT_RETURN: {
    cubec_statement_return_t ret = (cubec_statement_return_t)stmt;
    if (ret->expression) {
      semantic_type_t et = _check_expression(ctx, ret->expression);
      if (return_type && return_type->impl->kind != TYPE_ERROR &&
          et->impl->kind != TYPE_ERROR) {
        if (!semantic_type_can_implicit_convert(et, return_type)) {
          diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                               stmt->location,
                               "cannot return '%s' from function returning '%s'",
                               et->name ? et->name : "<anonymous>",
                               return_type->name ? return_type->name
                                                 : "<anonymous>");
          ctx->error_count++;
        }
      }
    } else {
      if (return_type && return_type->impl->kind != TYPE_VOID) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             stmt->location,
                             "missing return value in function returning '%s'",
                             return_type->name ? return_type->name
                                               : "<anonymous>");
        ctx->error_count++;
      }
    }
    break;
  }

  case CUBEC_NODE_STATEMENT_IF: {
    cubec_statement_if_t sif = (cubec_statement_if_t)stmt;
    semantic_type_t ct = _check_expression(ctx, sif->condition);
    if (!_is_bool_type(ct)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           stmt->location,
                           "if condition must be bool");
      ctx->error_count++;
    }
    _check_statement(ctx, sif->then_branch, return_type);
    if (sif->else_branch)
      _check_statement(ctx, sif->else_branch, return_type);
    break;
  }

  case CUBEC_NODE_STATEMENT_WHILE: {
    cubec_statement_while_t sw = (cubec_statement_while_t)stmt;
    semantic_type_t ct = _check_expression(ctx, sw->condition);
    if (!_is_bool_type(ct)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           stmt->location,
                           "while condition must be bool");
      ctx->error_count++;
    }
    ctx->loop_depth++;
    _check_statement(ctx, sw->body, return_type);
    ctx->loop_depth--;
    break;
  }

  case CUBEC_NODE_STATEMENT_DO_WHILE: {
    cubec_statement_do_while_t dw = (cubec_statement_do_while_t)stmt;
    ctx->loop_depth++;
    _check_statement(ctx, dw->body, return_type);
    ctx->loop_depth--;
    semantic_type_t ct = _check_expression(ctx, dw->condition);
    if (!_is_bool_type(ct)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           stmt->location,
                           "do-while condition must be bool");
      ctx->error_count++;
    }
    break;
  }

  case CUBEC_NODE_STATEMENT_FOR: {
    cubec_statement_for_t sf = (cubec_statement_for_t)stmt;
    scope_t saved = ctx->current_scope;
    ctx->current_scope = scope_create(ctx->allocator, ctx->current_scope,
                                       SCOPE_FOR, stmt->location);
    if (sf->init) _check_statement(ctx, sf->init, return_type);
    if (sf->condition) {
      semantic_type_t ct = _check_expression(ctx, sf->condition);
      if (!_is_bool_type(ct)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             stmt->location,
                             "for condition must be bool");
        ctx->error_count++;
      }
    }
    if (sf->increment) _check_expression(ctx, sf->increment);
    ctx->loop_depth++;
    _check_statement(ctx, sf->body, return_type);
    ctx->loop_depth--;
    ctx->current_scope = saved;
    break;
  }

  case CUBEC_NODE_STATEMENT_FOREACH: {
    cubec_statement_foreach_t sfe = (cubec_statement_foreach_t)stmt;
    semantic_type_t iter_type = _check_expression(ctx, sfe->iterator);
    scope_t saved = ctx->current_scope;
    ctx->current_scope = scope_create(ctx->allocator, ctx->current_scope,
                                       SCOPE_FOREACH, stmt->location);
    const char *vname = _ident_str(sfe->name);
    if (vname) {
      struct symbol *vsym = symbol_create(ctx->allocator, vname,
                                           SYMBOL_VARIABLE, stmt->location);
      /* TODO: derive element type from iterator */
      if (iter_type->impl->kind == TYPE_SLICE)
        vsym->variable.type = iter_type->impl->slice.element;
      else if (iter_type->impl->kind == TYPE_ARRAY)
        vsym->variable.type = iter_type->impl->array.element;
      else
        vsym->variable.type = ctx->error_type;
      vsym->variable.is_mutable = !sfe->is_const;
      vsym->state = SYMBOL_EVALUATED;
      scope_push_symbol(ctx->current_scope, vsym);
    }
    ctx->loop_depth++;
    _check_statement(ctx, sfe->body, return_type);
    ctx->loop_depth--;
    ctx->current_scope = saved;
    break;
  }

  case CUBEC_NODE_STATEMENT_BREAK:
    if (ctx->loop_depth <= 0) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           stmt->location,
                           "break statement not in loop");
      ctx->error_count++;
    }
    break;

  case CUBEC_NODE_STATEMENT_CONTINUE:
    if (ctx->loop_depth <= 0) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           stmt->location,
                           "continue statement not in loop");
      ctx->error_count++;
    }
    break;

  case CUBEC_NODE_STATEMENT_DEFER: {
    cubec_statement_defer_t sd = (cubec_statement_defer_t)stmt;
    _check_statement(ctx, sd->body, return_type);
    break;
  }

  case CUBEC_NODE_STATEMENT_SWITCH: {
    cubec_statement_switch_t ss = (cubec_statement_switch_t)stmt;
    _check_expression(ctx, ss->condition);
    if (ss->matches) {
      size_t mcount = vec_get_size(ss->matches);
      for (size_t i = 0; i < mcount; i++) {
        node_t match = (node_t)vec_get(ss->matches, i);
        if (match->kind == CUBEC_NODE_SWITCH_MATCH) {
          cubec_switch_match_t sm = (cubec_switch_match_t)match;
          if (sm->values) {
            size_t vcount = vec_get_size(sm->values);
            for (size_t j = 0; j < vcount; j++) {
              _check_expression(ctx, (node_t)vec_get(sm->values, j));
            }
          }
          _check_statement(ctx, sm->body, return_type);
        }
      }
    }
    break;
  }

  case CUBEC_NODE_STATEMENT_DECLARATION: {
    cubec_statement_declaration_t sdecl =
        (cubec_statement_declaration_t)stmt;
    cubec_declaration_variable_t vdecl =
        (cubec_declaration_variable_t)sdecl->declarator;
    if (!vdecl) break;

    const char *vname = _ident_str(vdecl->identifier);
    if (!vname) break;

    semantic_type_t var_type = NULL;
    if (vdecl->type)
      var_type = resolver_resolve_type(ctx, vdecl->type);
    if (!var_type && vdecl->expression) {
      var_type = _check_expression(ctx, vdecl->expression);
    }

    if (!var_type) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           stmt->location,
                           "cannot infer type for variable '%s'", vname);
      ctx->error_count++;
      var_type = ctx->error_type;
    }

    struct symbol *vsym = symbol_create(ctx->allocator, vname,
                                         SYMBOL_VARIABLE, stmt->location);
    vsym->variable.type = var_type;
    vsym->variable.is_comptime = sdecl->is_comptime;
    vsym->variable.is_mutable = true;
    vsym->state = SYMBOL_EVALUATED;
    scope_push_symbol(ctx->current_scope, vsym);
    break;
  }

  case CUBEC_NODE_STATEMENT_EMPTY:
    break;

  case CUBEC_NODE_STATEMENT_COMPTIME_BLOCK:
  case CUBEC_NODE_STATEMENT_COMPTIME_IF:
  case CUBEC_NODE_STATEMENT_COMPTIME_FOR:
    /* TODO: comptime evaluation */
    break;

  default:
    break;
  }
}

/* --- function body checker --- */

static void _check_function_body(checker_t ctx,
                                  cubec_statement_function_t node) {
  if (!node || !node->body) return;

  const char *name = _ident_str(node->name);
  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_FUNCTION || !sym->function.type) return;

  semantic_type_t ftype = sym->function.type;
  semantic_type_t return_type = ftype->impl->function.return_type;

  scope_t saved = ctx->current_scope;
  ctx->current_scope = scope_create(ctx->allocator, ctx->global_scope,
                                     SCOPE_FUNCTION, node->super.location);

  /* register parameters */
  if (node->arguments) {
    size_t count = vec_get_size(node->arguments);
    vec_t params = ftype->impl->function.params;
    for (size_t i = 0; i < count; i++) {
      node_t arg = (node_t)vec_get(node->arguments, i);
      if (arg->kind == CUBEC_NODE_FUNCTION_ARGUMENT) {
        cubec_function_argument_t farg = (cubec_function_argument_t)arg;
        const char *pname = _ident_str(farg->identifier);
        if (pname) {
          struct symbol *psym = symbol_create(ctx->allocator, pname,
                                               SYMBOL_VARIABLE, arg->location);
          psym->variable.type = (params && i < vec_get_size(params))
                                    ? (semantic_type_t)vec_get(params, i)
                                    : ctx->error_type;
          psym->variable.is_mutable = true;
          psym->state = SYMBOL_EVALUATED;
          scope_push_symbol(ctx->current_scope, psym);
        }
      }
    }
  }

  ctx->loop_depth = 0;
  _check_statement(ctx, node->body, return_type);

  ctx->current_scope = saved;
}

static void _check_struct_method_bodies(checker_t ctx,
                                         cubec_statement_struct_t node) {
  if (!node || !node->members) return;
  semantic_type_t t = NULL;

  const char *name = _ident_str(node->name);
  if (name) {
    struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
    if (sym && sym->kind == SYMBOL_TYPE) t = sym->type.type;
  }

  size_t count = vec_get_size(node->members);
  for (size_t i = 0; i < count; i++) {
    node_t member = (node_t)vec_get(node->members, i);
    if (member->kind == CUBEC_NODE_STATEMENT_FUNCTION) {
      cubec_statement_function_t mfn = (cubec_statement_function_t)member;
      if (!mfn->body) continue;

      const char *mname = _ident_str(mfn->name);
      semantic_type_t mtype = NULL;

      /* find method type from type_name_entry */
      if (t) {
        size_t mcount = vec_get_size(t->instance_methods);
        for (size_t j = 0; j < mcount; j++) {
          struct symbol *ms = (struct symbol *)vec_get(t->instance_methods, j);
          if (ms && ms->name && strcmp(ms->name, mname) == 0) {
            mtype = ms->function.type;
            break;
          }
        }
      }

      semantic_type_t return_type = mtype
          ? mtype->impl->function.return_type
          : ctx->builtin_void;

      scope_t saved = ctx->current_scope;
      ctx->current_scope = scope_create(ctx->allocator, ctx->global_scope,
                                         SCOPE_FUNCTION, mfn->super.location);

      /* register method parameters */
      if (mfn->arguments) {
        size_t acount = vec_get_size(mfn->arguments);
        vec_t params = mtype ? mtype->impl->function.params : NULL;
        for (size_t j = 0; j < acount; j++) {
          node_t arg = (node_t)vec_get(mfn->arguments, j);
          if (arg->kind == CUBEC_NODE_FUNCTION_ARGUMENT) {
            cubec_function_argument_t farg = (cubec_function_argument_t)arg;
            const char *pname = _ident_str(farg->identifier);
            if (pname) {
              struct symbol *psym = symbol_create(ctx->allocator, pname,
                                                   SYMBOL_VARIABLE,
                                                   arg->location);
              psym->variable.type = (params && j < vec_get_size(params))
                                      ? (semantic_type_t)vec_get(params, j)
                                      : ctx->error_type;
              psym->variable.is_mutable = true;
              psym->state = SYMBOL_EVALUATED;
              scope_push_symbol(ctx->current_scope, psym);
            }
          }
        }
      }

      ctx->loop_depth = 0;
      _check_statement(ctx, mfn->body, return_type);
      ctx->current_scope = saved;
    }
  }
}

static void _check_function_bodies(checker_t ctx, node_t program) {
  cubec_program_node_t prog = (cubec_program_node_t)program;
  if (!prog || !prog->statements) return;

  size_t count = vec_get_size(prog->statements);
  for (size_t i = 0; i < count; i++) {
    node_t stmt = (node_t)vec_get(prog->statements, i);
    if (!stmt) continue;

    switch (stmt->kind) {
    case CUBEC_NODE_STATEMENT_FUNCTION:
      _check_function_body(ctx, (cubec_statement_function_t)stmt);
      break;
    case CUBEC_NODE_STATEMENT_STRUCT:
      _check_struct_method_bodies(ctx, (cubec_statement_struct_t)stmt);
      break;
    /* TODO: union methods */
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

  /* Pass 3: Function body checking */
  _check_function_bodies(ctx, program);

  /* Pass 4: Generic instantiation — TODO */
}
