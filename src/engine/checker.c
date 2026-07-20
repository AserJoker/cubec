#include "engine/checker.h"
#include "engine/checker_type_util.h"
#include "engine/checker_collect.h"
#include "engine/checker_evaluate.h"
#include "engine/checker_check_expr.h"
#include "engine/checker_check_stmt.h"
#include "engine/comptime_eval.h"
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
#include "cubec/generic_param.h"
#include "cubec/decorator.h"
#include <string.h>

/* ===== builtin type registration ===== */

static semantic_type_t _register_builtin(checker_t ctx, const char *name,
                                          enum type_kind kind) {
  semantic_type_t t =
      semantic_type_create_named(ctx->allocator, name, kind);
  type_layout_compute(t, 8); /* default 64-bit */
  type_hash_ensure(t);

  /* Register in type_name_table */
  strmap_insert(ctx->type_name_table, name, t);

  /* Track for cleanup */
  vec_push(ctx->all_types, t);

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
  ctx->builtin_opaque  = _register_builtin(ctx, "opaque", TYPE_OPAQUE);
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

  /* Track all child scopes for cleanup */
  vec_init_t scopes_init = {.auto_dispose = true};
  ctx->all_scopes = allocator_create(allocator, &g_vec_type, &scopes_init);

  /* Create caches */
  strmap_init_t si = {.value_auto_dispose = false};
  ctx->module_cache =
      (strmap_t)allocator_create(allocator, &g_strmap_type, &si);
  ctx->type_name_table =
      (strmap_t)allocator_create(allocator, &g_strmap_type, &si);
  ctx->type_impl_cache =
      (strmap_t)allocator_create(allocator, &g_strmap_type, &si);

  /* Track all semantic types for cleanup */
  vec_init_t types_init = {.auto_dispose = true};
  ctx->all_types = allocator_create(allocator, &g_vec_type, &types_init);

  /* Init diagnostics */
  diagnostic_list_init_t dl_init = {.output = NULL};
  ctx->diagnostics = (diagnostic_list_t)allocator_create(
      allocator, &g_diagnostic_list_type, &dl_init);

  /* Init source cache */
  ctx->sources =
      (source_cache_t)allocator_create(allocator, &g_source_cache_type, NULL);

  /* Register builtin types */
  _register_builtins(ctx);

  /* Init builtin registry */
  ctx->builtin_table = builtin_table_create(allocator);
  builtin_table_init_defaults(ctx->builtin_table, ctx);

  /* Init comptime evaluator */
  ctx->comptime_eval = comptime_eval_create(allocator);

  /* Init test tracking */
  ctx->test_count = 0;
  ctx->test_fail_count = 0;
}

static void _checker_dispose(void *self, allocator_t allocator) {
  checker_t ctx = (checker_t)self;
  (void)allocator;

  /* Dispose comptime evaluator first (values reference semantic types) */
  comptime_eval_dispose(ctx->comptime_eval);
  allocator_free(allocator, &ctx->comptime_eval);

  /* Free all semantic types (includes builtin + user-defined types).
     This cascades: semantic_type dispose frees its type_impl and
     internal vecs (instance_methods, static_fields, etc.) with
     auto_dispose, which frees symbol objects inside them. */
  allocator_free(allocator, &ctx->all_types);

  /* Free child scopes (global_scope symbols already freed via all_types) */
  allocator_free(allocator, &ctx->all_scopes);
  allocator_free(allocator, &ctx->global_scope);

  allocator_free(allocator, &ctx->sources);
  allocator_free(allocator, &ctx->diagnostics);
  builtin_table_dispose(ctx->builtin_table, allocator);
  allocator_free(allocator, &ctx->type_impl_cache);
  allocator_free(allocator, &ctx->type_name_table);
  allocator_free(allocator, &ctx->module_cache);
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
  return ctx ? ctx->error_count : 0;
}

/* ===== Pass 1: Declaration Collection ===== */
