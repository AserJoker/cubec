#include "engine/checker.h"
#include "engine/symbol.h"
#include "engine/semantic_type.h"
#include "engine/type_hash.h"
#include "core/string.h"
#include "core/vec.h"
#include "cubec/program.h"
#include "cubec/node.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include "cubec/struct_field.h"
#include "cubec/enum_item.h"
#include "cubec/union_field.h"
#include "cubec/function_argument.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_enum.h"
#include "cubec/statement_union.h"
#include "cubec/statement_cunion.h"
#include "cubec/statement_function.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/declaration_variable.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ===== helpers for constructing AST nodes in tests ===== */

static location_t test_loc() {
  static location_t loc = {.filename = "<test>",
                            .begin = {1, 1, NULL},
                            .end = {1, 1, NULL}};
  return loc;
}

static node_t make_ident(allocator_t alloc, const char *name) {
  cubec_literal_identifier_init_t init = {.location = test_loc(),
                                           .parent = NULL,
                                           .value = name};
  return (node_t)allocator_create(alloc, &g_cubec_literal_identifier_type, &init);
}

static node_t make_numeric(allocator_t alloc, const char *value) {
  cubec_literal_numeric_init_t init = {.location = test_loc(),
                                        .parent = NULL,
                                        .value = value,
                                        .kind = CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                        .numeric_type = CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT};
  return (node_t)allocator_create(alloc, &g_cubec_literal_numeric_type, &init);
}

static node_t make_program(allocator_t alloc, vec_t statements) {
  cubec_program_node_init_t pinit = {.location = test_loc(), .parent = NULL};
  cubec_program_node_t prog =
      (cubec_program_node_t)allocator_create(alloc, &g_cubec_program_node_type,
                                              &pinit);
  /* Move items from statements vec into program's own statements vec */
  if (statements) {
    size_t count = vec_get_size(statements);
    for (size_t i = 0; i < count; i++) {
      vec_push(prog->statements, vec_get(statements, i));
    }
  }
  return (node_t)prog;
}

static vec_t make_vec(allocator_t alloc) {
  vec_init_t vi = {.auto_dispose = true};
  return (vec_t)allocator_create(alloc, &g_vec_type, &vi);
}

static node_t make_struct_stmt(allocator_t alloc, const char *name,
                               vec_t members) {
  node_t name_node = make_ident(alloc, name);
  cubec_statement_struct_init_t init = {.location = test_loc(),
                                        .parent = NULL,
                                        .is_export = false,
                                        .name = name_node,
                                        .generic_params = NULL,
                                        .members = members};
  return (node_t)allocator_create(alloc, &g_cubec_statement_struct_type, &init);
}

static node_t make_struct_field(allocator_t alloc, const char *name,
                                node_t type) {
  node_t name_node = make_ident(alloc, name);
  cubec_struct_field_init_t init = {.location = test_loc(),
                                    .parent = NULL,
                                    .is_pub = false,
                                    .name = name_node,
                                    .type = type};
  return (node_t)allocator_create(alloc, &g_cubec_struct_field_type, &init);
}

static node_t make_enum_stmt(allocator_t alloc, const char *name, vec_t items) {
  node_t name_node = make_ident(alloc, name);
  cubec_statement_enum_init_t init = {.location = test_loc(),
                                      .parent = NULL,
                                      .is_export = false,
                                      .name = name_node,
                                      .items = items};
  return (node_t)allocator_create(alloc, &g_cubec_statement_enum_type, &init);
}

static node_t make_enum_item(allocator_t alloc, const char *name) {
  node_t name_node = make_ident(alloc, name);
  cubec_enum_item_init_t init = {.location = test_loc(),
                                  .parent = NULL,
                                  .name = name_node,
                                  .type = NULL,
                                  .value = NULL};
  return (node_t)allocator_create(alloc, &g_cubec_enum_item_type, &init);
}

static node_t make_union_stmt(allocator_t alloc, const char *name,
                              vec_t members) {
  node_t name_node = make_ident(alloc, name);
  cubec_statement_union_init_t init = {.location = test_loc(),
                                       .parent = NULL,
                                       .is_export = false,
                                       .name = name_node,
                                       .generic_params = NULL,
                                       .members = members};
  return (node_t)allocator_create(alloc, &g_cubec_statement_union_type, &init);
}

static node_t make_union_field(allocator_t alloc, const char *name,
                               node_t type) {
  node_t name_node = make_ident(alloc, name);
  cubec_union_field_init_t init = {.location = test_loc(),
                                   .parent = NULL,
                                   .name = name_node,
                                   .type = type};
  return (node_t)allocator_create(alloc, &g_cubec_union_field_type, &init);
}

static node_t make_cunion_stmt(allocator_t alloc, const char *name,
                               vec_t fields) {
  node_t name_node = make_ident(alloc, name);
  cubec_statement_cunion_init_t init = {.location = test_loc(),
                                         .parent = NULL,
                                         .name = name_node,
                                         .fields = fields};
  return (node_t)allocator_create(alloc, &g_cubec_statement_cunion_type,
                                   &init);
}

static node_t make_func_stmt(allocator_t alloc, const char *name,
                             node_t ret_type, vec_t args) {
  node_t name_node = make_ident(alloc, name);
  cubec_statement_function_init_t init = {.location = test_loc(),
                                           .parent = NULL,
                                           .is_export = false,
                                           .is_inline = false,
                                           .is_extern = false,
                                           .is_builtin = false,
                                           .is_comptime = false,
                                           .is_c_variadic = false,
                                           .name = name_node,
                                           .generic_params = NULL,
                                           .arguments = args,
                                           .return_type = ret_type,
                                           .body = NULL};
  return (node_t)allocator_create(alloc, &g_cubec_statement_function_type,
                                   &init);
}

static node_t make_func_arg(allocator_t alloc, const char *name, node_t type) {
  node_t name_node = make_ident(alloc, name);
  cubec_function_argument_init_t init = {.location = test_loc(),
                                          .identifier = name_node,
                                          .type = type};
  return (node_t)allocator_create(alloc, &g_cubec_function_argument_type,
                                   &init);
}

/* Note: cubec_function_argument_init_t has no parent field per header */

static node_t make_var_decl(allocator_t alloc, const char *name, node_t type,
                            node_t expr) {
  node_t name_node = make_ident(alloc, name);
  cubec_declaration_variable_init_t dv_init = {.location = test_loc(),
                                                .parent = NULL,
                                                .identifier = name_node,
                                                .type = type,
                                                .expression = expr};
  node_t decl_node = (node_t)allocator_create(
      alloc, &g_cubec_declaration_variable_type, &dv_init);

  cubec_statement_declaration_init_t sd_init = {.location = test_loc(),
                                                 .parent = NULL,
                                                 .is_export = false,
                                                 .is_extern = false,
                                                 .is_builtin = false,
                                                 .is_comptime = false,
                                                 .declarator = decl_node};
  return (node_t)allocator_create(alloc, &g_cubec_statement_declaration_type,
                                   &sd_init);
}

static node_t make_type_alias(allocator_t alloc, const char *name,
                              node_t type_value) {
  node_t name_node = make_ident(alloc, name);
  cubec_statement_declaration_type_init_t init = {.location = test_loc(),
                                                    .parent = NULL,
                                                    .is_export = false,
                                                    .is_builtin = false,
                                                    .name = name_node,
                                                    .params = NULL,
                                                    .type_value = type_value};
  return (node_t)allocator_create(
      alloc, &g_cubec_statement_decltype, &init);
}

/* ===== existing Pass 1 tests ===== */

class dt_checker : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

TEST_F(dt_checker, create_and_dispose) {
  checker_t ctx = checker_create(allocator);
  ASSERT_NE(ctx, nullptr);
  EXPECT_NE(ctx->global_scope, nullptr);
  EXPECT_NE(ctx->diagnostics, nullptr);
  EXPECT_NE(ctx->sources, nullptr);
  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
}

TEST_F(dt_checker, builtin_types_registered) {
  checker_t ctx = checker_create(allocator);

  /* Check builtin types are non-null */
  EXPECT_NE(ctx->builtin_void, nullptr);
  EXPECT_NE(ctx->builtin_bool, nullptr);
  EXPECT_NE(ctx->builtin_i32, nullptr);
  EXPECT_NE(ctx->builtin_f64, nullptr);
  EXPECT_NE(ctx->builtin_nil, nullptr);
  EXPECT_NE(ctx->error_type, nullptr);

  /* Check kinds */
  EXPECT_EQ(semantic_type_get_kind(ctx->builtin_void), TYPE_VOID);
  EXPECT_EQ(semantic_type_get_kind(ctx->builtin_bool), TYPE_BOOL);
  EXPECT_EQ(semantic_type_get_kind(ctx->builtin_i32), TYPE_I32);
  EXPECT_EQ(semantic_type_get_kind(ctx->builtin_f64), TYPE_F64);
  EXPECT_EQ(semantic_type_get_kind(ctx->builtin_string), TYPE_STRING);
  EXPECT_EQ(semantic_type_get_kind(ctx->builtin_nil), TYPE_NIL);
  EXPECT_EQ(semantic_type_get_kind(ctx->error_type), TYPE_ERROR);

  checker_dispose(ctx);
}

TEST_F(dt_checker, builtin_types_in_scope) {
  checker_t ctx = checker_create(allocator);

  /* Check that builtins are in global scope */
  struct symbol *sym = scope_lookup(ctx->global_scope, "i32");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_TYPE);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);
  EXPECT_EQ(sym->type.type, ctx->builtin_i32);

  sym = scope_lookup(ctx->global_scope, "void");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_TYPE);
  EXPECT_EQ(sym->type.type, ctx->builtin_void);

  checker_dispose(ctx);
}

TEST_F(dt_checker, builtin_types_in_name_table) {
  checker_t ctx = checker_create(allocator);

  void *found = strmap_find(ctx->type_name_table, "i32");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ((semantic_type_t)found, ctx->builtin_i32);

  found = strmap_find(ctx->type_name_table, "void");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ((semantic_type_t)found, ctx->builtin_void);

  checker_dispose(ctx);
}

TEST_F(dt_checker, builtin_type_sizes) {
  checker_t ctx = checker_create(allocator);

  EXPECT_EQ(semantic_type_get_size(ctx->builtin_void), 0u);
  EXPECT_EQ(semantic_type_get_size(ctx->builtin_bool), 1u);
  EXPECT_EQ(semantic_type_get_size(ctx->builtin_i8), 1u);
  EXPECT_EQ(semantic_type_get_size(ctx->builtin_i32), 4u);
  EXPECT_EQ(semantic_type_get_size(ctx->builtin_i64), 8u);
  EXPECT_EQ(semantic_type_get_size(ctx->builtin_f32), 4u);
  EXPECT_EQ(semantic_type_get_size(ctx->builtin_f64), 8u);
  EXPECT_EQ(semantic_type_get_size(ctx->builtin_string), 16u); /* ptr+len */

  checker_dispose(ctx);
}

/* ===== Pass 2 tests ===== */

TEST_F(dt_checker, pass2_struct_basic_debug) {
  /* Minimal: just create checker + empty struct */
  vec_t stmts = make_vec(allocator);
  vec_push(stmts, make_struct_stmt(allocator, "Point", make_vec(allocator)));

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, make_program(allocator, stmts));

  struct symbol *sym = scope_lookup(ctx->global_scope, "Point");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_TYPE);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);

  checker_dispose(ctx);
}

TEST_F(dt_checker, pass2_struct_basic) {
  /* struct Point { x: f64; y: f64; } */
  vec_t members = make_vec(allocator);
  node_t fx = make_struct_field(allocator, "x", make_ident(allocator, "f64"));
  node_t fy = make_struct_field(allocator, "y", make_ident(allocator, "f64"));
  vec_push(members, fx);
  vec_push(members, fy);

  vec_t stmts = make_vec(allocator);
  vec_push(stmts, make_struct_stmt(allocator, "Point", members));

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, make_program(allocator, stmts));

  /* Symbol should be EVALUATED */
  struct symbol *sym = scope_lookup(ctx->global_scope, "Point");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_TYPE);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);

  /* Type should be complete */
  semantic_type_t t = sym->type.type;
  ASSERT_NE(t, nullptr);
  EXPECT_FALSE(semantic_type_is_incomplete(t));
  EXPECT_EQ(semantic_type_get_kind(t), TYPE_STRUCT);

  /* Layout: 2x f64 = 16 bytes, alignment 8 */
  EXPECT_EQ(semantic_type_get_size(t), 16u);
  EXPECT_EQ(semantic_type_get_alignment(t), 8u);

  /* Fields */
  vec_t fields = t->impl->struct_type.fields;
  ASSERT_NE(fields, nullptr);
  EXPECT_EQ(vec_get_size(fields), 2u);

  struct symbol *f0 = (struct symbol *)vec_get(fields, 0);
  EXPECT_STREQ(f0->name, "x");
  EXPECT_EQ(f0->field.type, ctx->builtin_f64);
  EXPECT_EQ(f0->field.index, 0u);

  struct symbol *f1 = (struct symbol *)vec_get(fields, 1);
  EXPECT_STREQ(f1->name, "y");
  EXPECT_EQ(f1->field.type, ctx->builtin_f64);
  EXPECT_EQ(f1->field.index, 1u);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
}

TEST_F(dt_checker, pass2_enum_basic) {
  /* enum Color { Red, Green, Blue } */
  vec_t items = make_vec(allocator);
  vec_push(items, make_enum_item(allocator, "Red"));
  vec_push(items, make_enum_item(allocator, "Green"));
  vec_push(items, make_enum_item(allocator, "Blue"));

  vec_t stmts = make_vec(allocator);
  vec_push(stmts, make_enum_stmt(allocator, "Color", items));

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, make_program(allocator, stmts));

  struct symbol *sym = scope_lookup(ctx->global_scope, "Color");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_TYPE);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);

  semantic_type_t t = sym->type.type;
  ASSERT_NE(t, nullptr);
  EXPECT_FALSE(semantic_type_is_incomplete(t));
  EXPECT_EQ(semantic_type_get_kind(t), TYPE_ENUM);
  EXPECT_EQ(t->impl->enum_type.backing_type, ctx->builtin_i32);
  EXPECT_EQ(semantic_type_get_size(t), 4u);

  /* Enum items */
  vec_t ev = t->impl->enum_type.items;
  ASSERT_EQ(vec_get_size(ev), 3u);

  struct symbol *e0 = (struct symbol *)vec_get(ev, 0);
  EXPECT_STREQ(e0->name, "Red");
  EXPECT_EQ(e0->enum_item.value, 0);

  struct symbol *e1 = (struct symbol *)vec_get(ev, 1);
  EXPECT_STREQ(e1->name, "Green");
  EXPECT_EQ(e1->enum_item.value, 1);

  struct symbol *e2 = (struct symbol *)vec_get(ev, 2);
  EXPECT_STREQ(e2->name, "Blue");
  EXPECT_EQ(e2->enum_item.value, 2);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
}

TEST_F(dt_checker, pass2_union_basic) {
  /* union Value { int_val: i64; flt_val: f64; } */
  vec_t members = make_vec(allocator);
  vec_push(members, make_union_field(allocator, "int_val",
                                      make_ident(allocator, "i64")));
  vec_push(members, make_union_field(allocator, "flt_val",
                                      make_ident(allocator, "f64")));

  vec_t stmts = make_vec(allocator);
  vec_push(stmts, make_union_stmt(allocator, "Value", members));

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, make_program(allocator, stmts));

  struct symbol *sym = scope_lookup(ctx->global_scope, "Value");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);

  semantic_type_t t = sym->type.type;
  EXPECT_FALSE(semantic_type_is_incomplete(t));
  EXPECT_EQ(semantic_type_get_kind(t), TYPE_UNION);
  /* Union: size = max(8, 8) = 8, alignment = 8 */
  EXPECT_EQ(semantic_type_get_size(t), 8u);
  EXPECT_EQ(semantic_type_get_alignment(t), 8u);

  vec_t fields = t->impl->struct_type.fields;
  ASSERT_EQ(vec_get_size(fields), 2u);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
}

TEST_F(dt_checker, pass2_cunion_basic) {
  /* cunion Data { a: i32; b: f64; } */
  vec_t fields = make_vec(allocator);
  vec_push(fields, make_struct_field(allocator, "a", make_ident(allocator, "i32")));
  vec_push(fields, make_struct_field(allocator, "b", make_ident(allocator, "f64")));

  vec_t stmts = make_vec(allocator);
  vec_push(stmts, make_cunion_stmt(allocator, "Data", fields));

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, make_program(allocator, stmts));

  struct symbol *sym = scope_lookup(ctx->global_scope, "Data");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);

  semantic_type_t t = sym->type.type;
  EXPECT_FALSE(semantic_type_is_incomplete(t));
  EXPECT_EQ(semantic_type_get_kind(t), TYPE_CUNION);
  /* cunion: size = max(4, 8) = 8, alignment = 8 */
  EXPECT_EQ(semantic_type_get_size(t), 8u);
  EXPECT_EQ(semantic_type_get_alignment(t), 8u);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
}

TEST_F(dt_checker, pass2_function_basic) {
  /* func add(a: i32, b: i32): i32 {} */
  vec_t args = make_vec(allocator);
  vec_push(args, make_func_arg(allocator, "a", make_ident(allocator, "i32")));
  vec_push(args, make_func_arg(allocator, "b", make_ident(allocator, "i32")));

  vec_t stmts = make_vec(allocator);
  vec_push(stmts, make_func_stmt(allocator, "add",
                                  make_ident(allocator, "i32"), args));

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, make_program(allocator, stmts));

  struct symbol *sym = scope_lookup(ctx->global_scope, "add");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_FUNCTION);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);

  semantic_type_t ft = sym->function.type;
  ASSERT_NE(ft, nullptr);
  EXPECT_EQ(semantic_type_get_kind(ft), TYPE_FUNCTION);
  EXPECT_EQ(ft->impl->function.return_type, ctx->builtin_i32);
  EXPECT_EQ(vec_get_size(ft->impl->function.params), 2u);

  /* Check param types */
  semantic_type_t p0 = (semantic_type_t)vec_get(ft->impl->function.params, 0);
  EXPECT_EQ(p0, ctx->builtin_i32);
  semantic_type_t p1 = (semantic_type_t)vec_get(ft->impl->function.params, 1);
  EXPECT_EQ(p1, ctx->builtin_i32);

  EXPECT_FALSE(ft->impl->function.is_variadic);
  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
}

TEST_F(dt_checker, pass2_variable_typed) {
  /* var x: i32 = 42 */
  vec_t stmts = make_vec(allocator);
  vec_push(stmts, make_var_decl(allocator, "x",
                                 make_ident(allocator, "i32"),
                                 make_numeric(allocator, "42")));

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, make_program(allocator, stmts));

  struct symbol *sym = scope_lookup(ctx->global_scope, "x");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_VARIABLE);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);
  EXPECT_EQ(sym->variable.type, ctx->builtin_i32);
  EXPECT_TRUE(sym->variable.is_mutable);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
}

TEST_F(dt_checker, pass2_variable_inferred_literal) {
  /* var n = 42 — type inferred as i32 */
  vec_t stmts = make_vec(allocator);
  vec_push(stmts, make_var_decl(allocator, "n", NULL,
                                 make_numeric(allocator, "42")));

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, make_program(allocator, stmts));

  struct symbol *sym = scope_lookup(ctx->global_scope, "n");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);
  EXPECT_EQ(sym->variable.type, ctx->builtin_i32);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
}

TEST_F(dt_checker, pass2_type_alias) {
  /* type MyInt = i32 */
  vec_t stmts = make_vec(allocator);
  vec_push(stmts, make_type_alias(allocator, "MyInt",
                                   make_ident(allocator, "i32")));

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, make_program(allocator, stmts));

  struct symbol *sym = scope_lookup(ctx->global_scope, "MyInt");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_TYPE);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);
  EXPECT_EQ(sym->type.type, ctx->builtin_i32);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
}

TEST_F(dt_checker, pass2_duplicate_continues) {
  /* Two structs with same name — Pass 1 errors, Pass 2 skips */
  vec_t stmts = make_vec(allocator);
  vec_push(stmts, make_struct_stmt(allocator, "Foo", make_vec(allocator)));
  vec_push(stmts, make_struct_stmt(allocator, "Foo", make_vec(allocator)));

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, make_program(allocator, stmts));

  /* First Foo should be resolved, second should error in Pass 1 */
  EXPECT_EQ(checker_get_error_count(ctx), 1);

  /* First Foo should still be EVALUATED */
  struct symbol *sym = scope_lookup(ctx->global_scope, "Foo");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);

  checker_dispose(ctx);
}

TEST_F(dt_checker, pass2_incomplete_type_in_var) {
  /* Use void as value type — void is incomplete, should produce an error */
  vec_t stmts = make_vec(allocator);
  vec_push(stmts, make_var_decl(allocator, "x",
                                 make_ident(allocator, "void"), NULL));

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, make_program(allocator, stmts));

  /* Should have an error for incomplete type */
  EXPECT_GT(checker_get_error_count(ctx), 0);

  checker_dispose(ctx);
}

TEST_F(dt_checker, pass2_function_with_custom_types) {
  /* struct Point { x: f64; y: f64; }
     func origin(): Point {} */
  vec_t members = make_vec(allocator);
  vec_push(members, make_struct_field(allocator, "x", make_ident(allocator, "f64")));
  vec_push(members, make_struct_field(allocator, "y", make_ident(allocator, "f64")));

  vec_t stmts = make_vec(allocator);
  vec_push(stmts, make_struct_stmt(allocator, "Point", members));
  vec_push(stmts, make_func_stmt(allocator, "origin",
                                  make_ident(allocator, "Point"),
                                  make_vec(allocator)));

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, make_program(allocator, stmts));

  /* Check function uses resolved Point type */
  struct symbol *fn_sym = scope_lookup(ctx->global_scope, "origin");
  ASSERT_NE(fn_sym, nullptr);
  EXPECT_EQ(fn_sym->state, SYMBOL_EVALUATED);

  semantic_type_t ft = fn_sym->function.type;
  ASSERT_NE(ft, nullptr);

  struct symbol *pt_sym = scope_lookup(ctx->global_scope, "Point");
  ASSERT_NE(pt_sym, nullptr);
  EXPECT_EQ(ft->impl->function.return_type, pt_sym->type.type);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
}
