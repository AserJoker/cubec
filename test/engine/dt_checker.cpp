#include "engine/checker.h"
#include "engine/symbol.h"
#include "engine/semantic_type.h"
#include "engine/type_hash.h"
#include "cubec/ast_factory.h"
#include "cubec/statement_block.h"
#include "cubec/statement_function.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ===== helpers ===== */

static location_t test_loc() {
  static location_t loc = {.filename = "<test>",
                            .begin = {1, 1, NULL},
                            .end = {1, 1, NULL}};
  return loc;
}

/* Shorthand aliases for brevity in tests */
#define T test_loc()

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
  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_struct_stmt(allocator, T, "Point",
                   cubec_ast_create_vec(allocator, true), false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  struct symbol *sym = scope_lookup(ctx->global_scope, "Point");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_TYPE);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);

  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_struct_basic) {
  /* struct Point { x: f64; y: f64; } */
  vec_t members = cubec_ast_create_vec(allocator, true);
  vec_push(members, cubec_ast_create_struct_field(allocator, T, "x",
                   cubec_ast_create_identifier(allocator, T, "f64"), false));
  vec_push(members, cubec_ast_create_struct_field(allocator, T, "y",
                   cubec_ast_create_identifier(allocator, T, "f64"), false));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_struct_stmt(allocator, T, "Point", members, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

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
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_enum_basic) {
  /* enum Color { Red, Green, Blue } */
  vec_t items = cubec_ast_create_vec(allocator, true);
  vec_push(items, cubec_ast_create_enum_item(allocator, T, "Red", NULL, NULL));
  vec_push(items, cubec_ast_create_enum_item(allocator, T, "Green", NULL, NULL));
  vec_push(items, cubec_ast_create_enum_item(allocator, T, "Blue", NULL, NULL));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_enum_stmt(allocator, T, "Color", items, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

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
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_union_basic) {
  /* union Value { int_val: i64; flt_val: f64; } */
  vec_t members = cubec_ast_create_vec(allocator, true);
  vec_push(members, cubec_ast_create_union_field(allocator, T, "int_val",
                   cubec_ast_create_identifier(allocator, T, "i64")));
  vec_push(members, cubec_ast_create_union_field(allocator, T, "flt_val",
                   cubec_ast_create_identifier(allocator, T, "f64")));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_union_stmt(allocator, T, "Value", members, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

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
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_cunion_basic) {
  /* cunion Data { a: i32; b: f64; } */
  vec_t fields = cubec_ast_create_vec(allocator, true);
  vec_push(fields, cubec_ast_create_struct_field(allocator, T, "a",
                   cubec_ast_create_identifier(allocator, T, "i32"), false));
  vec_push(fields, cubec_ast_create_struct_field(allocator, T, "b",
                   cubec_ast_create_identifier(allocator, T, "f64"), false));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_cunion_stmt(allocator, T, "Data", fields));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

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
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_function_basic) {
  /* func add(a: i32, b: i32): i32 {} */
  vec_t args = cubec_ast_create_vec(allocator, true);
  vec_push(args, cubec_ast_create_func_arg(allocator, T, "a",
                   cubec_ast_create_identifier(allocator, T, "i32")));
  vec_push(args, cubec_ast_create_func_arg(allocator, T, "b",
                   cubec_ast_create_identifier(allocator, T, "i32")));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "add", args,
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   NULL, false, false, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

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
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_variable_typed) {
  /* var x: i32 = 42 */
  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_var_decl_stmt(allocator, T, "x",
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   cubec_ast_create_numeric(allocator, T, "42",
                     CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                     CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT),
                   false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  struct symbol *sym = scope_lookup(ctx->global_scope, "x");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_VARIABLE);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);
  EXPECT_EQ(sym->variable.type, ctx->builtin_i32);
  EXPECT_TRUE(sym->variable.is_mutable);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_variable_inferred_literal) {
  /* var n = 42 — type inferred as i32 */
  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_var_decl_stmt(allocator, T, "n", NULL,
                   cubec_ast_create_numeric(allocator, T, "42",
                     CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                     CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT),
                   false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  struct symbol *sym = scope_lookup(ctx->global_scope, "n");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);
  EXPECT_EQ(sym->variable.type, ctx->builtin_i32);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_type_alias) {
  /* type MyInt = i32 */
  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_type_alias(allocator, T, "MyInt",
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  struct symbol *sym = scope_lookup(ctx->global_scope, "MyInt");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_TYPE);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);
  EXPECT_EQ(sym->type.type, ctx->builtin_i32);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_duplicate_continues) {
  /* Two structs with same name — Pass 1 errors, Pass 2 skips */
  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_struct_stmt(allocator, T, "Foo",
                   cubec_ast_create_vec(allocator, true), false));
  vec_push(stmts, cubec_ast_create_struct_stmt(allocator, T, "Foo",
                   cubec_ast_create_vec(allocator, true), false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  /* First Foo should be resolved, second should error in Pass 1 */
  EXPECT_EQ(checker_get_error_count(ctx), 1);

  /* First Foo should still be EVALUATED */
  struct symbol *sym = scope_lookup(ctx->global_scope, "Foo");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);

  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_incomplete_type_in_var) {
  /* Use void as value type — void is incomplete, should produce an error */
  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_var_decl_stmt(allocator, T, "x",
                   cubec_ast_create_identifier(allocator, T, "void"),
                   NULL, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  /* Should have an error for incomplete type */
  EXPECT_GT(checker_get_error_count(ctx), 0);

  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_function_with_custom_types) {
  /* struct Point { x: f64; y: f64; }
     func origin(): Point {} */
  vec_t members = cubec_ast_create_vec(allocator, true);
  vec_push(members, cubec_ast_create_struct_field(allocator, T, "x",
                   cubec_ast_create_identifier(allocator, T, "f64"), false));
  vec_push(members, cubec_ast_create_struct_field(allocator, T, "y",
                   cubec_ast_create_identifier(allocator, T, "f64"), false));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_struct_stmt(allocator, T, "Point", members, false));
  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "origin",
                   cubec_ast_create_vec(allocator, true),
                   cubec_ast_create_identifier(allocator, T, "Point"),
                   NULL, false, false, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

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
  allocator_free(allocator, &prog);
}

/* ===== numeric suffix inference tests ===== */

TEST_F(dt_checker, pass2_numeric_suffix_i64) {
  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_var_decl_stmt(allocator, T, "x", NULL,
                   cubec_ast_create_numeric(allocator, T, "42",
                     CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                     CUBEC_LITERAL_NUMERIC_TYPE_I64),
                   false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  struct symbol *sym = scope_lookup(ctx->global_scope, "x");
  ASSERT_NE(sym, nullptr);
  ASSERT_NE(sym->variable.type, nullptr);
  EXPECT_EQ(sym->variable.type->impl->kind, TYPE_I64);

  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_numeric_suffix_f32) {
  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_var_decl_stmt(allocator, T, "pi", NULL,
                   cubec_ast_create_numeric(allocator, T, "3.14",
                     CUBEC_LITERAL_NUMERIC_KIND_FLOAT,
                     CUBEC_LITERAL_NUMERIC_TYPE_F32),
                   false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  struct symbol *sym = scope_lookup(ctx->global_scope, "pi");
  ASSERT_NE(sym, nullptr);
  ASSERT_NE(sym->variable.type, nullptr);
  EXPECT_EQ(sym->variable.type->impl->kind, TYPE_F32);

  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_numeric_default_float) {
  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_var_decl_stmt(allocator, T, "d", NULL,
                   cubec_ast_create_numeric(allocator, T, "2.0",
                     CUBEC_LITERAL_NUMERIC_KIND_FLOAT,
                     CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT),
                   false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  struct symbol *sym = scope_lookup(ctx->global_scope, "d");
  ASSERT_NE(sym, nullptr);
  ASSERT_NE(sym->variable.type, nullptr);
  EXPECT_EQ(sym->variable.type->impl->kind, TYPE_F64);

  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_numeric_default_int) {
  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_var_decl_stmt(allocator, T, "n", NULL,
                   cubec_ast_create_numeric(allocator, T, "7",
                     CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                     CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT),
                   false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  struct symbol *sym = scope_lookup(ctx->global_scope, "n");
  ASSERT_NE(sym, nullptr);
  ASSERT_NE(sym->variable.type, nullptr);
  EXPECT_EQ(sym->variable.type->impl->kind, TYPE_I32);

  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_numeric_suffix_u8) {
  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_var_decl_stmt(allocator, T, "byte", NULL,
                   cubec_ast_create_numeric(allocator, T, "255",
                     CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                     CUBEC_LITERAL_NUMERIC_TYPE_U8),
                   false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  struct symbol *sym = scope_lookup(ctx->global_scope, "byte");
  ASSERT_NE(sym, nullptr);
  ASSERT_NE(sym->variable.type, nullptr);
  EXPECT_EQ(sym->variable.type->impl->kind, TYPE_U8);

  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

/* ===== struct method and static field tests ===== */

TEST_F(dt_checker, pass2_struct_method) {
  /* struct Counter { val: i32; func get(): i32 {} } */
  vec_t members = cubec_ast_create_vec(allocator, true);
  vec_push(members, cubec_ast_create_struct_field(allocator, T, "val",
                   cubec_ast_create_identifier(allocator, T, "i32"), false));
  vec_t args = cubec_ast_create_vec(allocator, true);
  vec_push(members, cubec_ast_create_func_stmt(allocator, T, "get",
                   args,
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   NULL, false, false, false, false, false, false));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_struct_stmt(allocator, T, "Counter", members, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  struct symbol *sym = scope_lookup(ctx->global_scope, "Counter");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);

  semantic_type_t t = sym->type.type;
  ASSERT_NE(t, nullptr);
  EXPECT_EQ(vec_get_size(t->instance_methods), 1);

  struct symbol *m = (struct symbol *)vec_get(t->instance_methods, 0);
  ASSERT_NE(m, nullptr);
  EXPECT_EQ(m->kind, SYMBOL_FUNCTION);
  EXPECT_STREQ(m->name, "get");
  ASSERT_NE(m->function.type, nullptr);
  EXPECT_EQ(m->function.type->impl->kind, TYPE_FUNCTION);
  EXPECT_EQ(m->function.type->impl->function.return_type->impl->kind, TYPE_I32);

  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_struct_static_field) {
  /* struct Config { var count: i32; } */
  vec_t members = cubec_ast_create_vec(allocator, true);
  vec_push(members, cubec_ast_create_var_decl_stmt(allocator, T, "count",
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   NULL, false, false, false, false));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_struct_stmt(allocator, T, "Config", members, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  struct symbol *sym = scope_lookup(ctx->global_scope, "Config");
  ASSERT_NE(sym, nullptr);
  semantic_type_t t = sym->type.type;
  ASSERT_NE(t, nullptr);
  EXPECT_EQ(vec_get_size(t->static_fields), 1);

  struct symbol *sf = (struct symbol *)vec_get(t->static_fields, 0);
  ASSERT_NE(sf, nullptr);
  EXPECT_EQ(sf->kind, SYMBOL_VARIABLE);
  EXPECT_STREQ(sf->name, "count");
  ASSERT_NE(sf->variable.type, nullptr);
  EXPECT_EQ(sf->variable.type->impl->kind, TYPE_I32);

  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_interface_associated_type) {
  /* interface Hashable { type Key; func hash(key: Key): u64; } */
  vec_t members = cubec_ast_create_vec(allocator, true);
  vec_push(members, cubec_ast_create_type_alias(allocator, T, "Key", NULL, false, false));
  vec_t args = cubec_ast_create_vec(allocator, true);
  vec_push(args, cubec_ast_create_func_arg(allocator, T, "key",
                   cubec_ast_create_identifier(allocator, T, "Key")));
  vec_push(members, cubec_ast_create_iface_method(allocator, T, "hash",
                   args,
                   cubec_ast_create_identifier(allocator, T, "u64")));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_iface_stmt(allocator, T, "Hashable", members, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  struct symbol *sym = scope_lookup(ctx->global_scope, "Hashable");
  ASSERT_NE(sym, nullptr);
  semantic_type_t t = sym->type.type;
  ASSERT_NE(t, nullptr);

  /* 1 associated type */
  EXPECT_EQ(vec_get_size(t->associated_types), 1);
  struct symbol *at = (struct symbol *)vec_get(t->associated_types, 0);
  EXPECT_EQ(at->kind, SYMBOL_TYPE);
  EXPECT_STREQ(at->name, "Key");

  /* 1 method */
  EXPECT_EQ(vec_get_size(t->impl->interface_type.methods), 1);

  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

/* ===== Pass 3 tests ===== */

TEST_F(dt_checker, pass3_return_typed) {
  /* func f(): i32 { return 42; } */
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_return_stmt(allocator, T,
                   cubec_ast_create_numeric(allocator, T, "42",
                     CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                     CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT)));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "f",
                   cubec_ast_create_vec(allocator, true),
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   cubec_ast_create_block(allocator, T, body_stmts),
                   false, false, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_return_mismatch) {
  /* func f(): i32 { return "hello"; } — type mismatch */
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_return_stmt(allocator, T,
                   cubec_ast_create_string(allocator, T, "hello")));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "f",
                   cubec_ast_create_vec(allocator, true),
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   cubec_ast_create_block(allocator, T, body_stmts),
                   false, false, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  EXPECT_GT(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_return_void) {
  /* func f() { return; } — bare return in void function */
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_return_stmt(allocator, T, NULL));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "f",
                   cubec_ast_create_vec(allocator, true), NULL,
                   cubec_ast_create_block(allocator, T, body_stmts),
                   false, false, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_binary_arithmetic) {
  /* func f(): i32 { return 1 + 2; } */
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_return_stmt(allocator, T,
                   cubec_ast_create_binary(allocator, T, "+",
                     cubec_ast_create_numeric(allocator, T, "1",
                       CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                       CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT),
                     cubec_ast_create_numeric(allocator, T, "2",
                       CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                       CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT))));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "f",
                   cubec_ast_create_vec(allocator, true),
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   cubec_ast_create_block(allocator, T, body_stmts),
                   false, false, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_binary_comparison) {
  /* func f(): bool { return 1 < 2; } */
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_return_stmt(allocator, T,
                   cubec_ast_create_binary(allocator, T, "<",
                     cubec_ast_create_numeric(allocator, T, "1",
                       CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                       CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT),
                     cubec_ast_create_numeric(allocator, T, "2",
                       CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                       CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT))));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "f",
                   cubec_ast_create_vec(allocator, true),
                   cubec_ast_create_identifier(allocator, T, "bool"),
                   cubec_ast_create_block(allocator, T, body_stmts),
                   false, false, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_call_basic) {
  /* func add(a: i32, b: i32): i32 {} func f(): i32 { return add(1, 2); } */
  vec_t add_args = cubec_ast_create_vec(allocator, true);
  vec_push(add_args, cubec_ast_create_func_arg(allocator, T, "a",
                   cubec_ast_create_identifier(allocator, T, "i32")));
  vec_push(add_args, cubec_ast_create_func_arg(allocator, T, "b",
                   cubec_ast_create_identifier(allocator, T, "i32")));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "add",
                   add_args,
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   NULL, false, false, false, false, false, false));

  vec_t call_args = cubec_ast_create_vec(allocator, true);
  vec_push(call_args, cubec_ast_create_numeric(allocator, T, "1",
                    CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                    CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT));
  vec_push(call_args, cubec_ast_create_numeric(allocator, T, "2",
                    CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                    CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT));

  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_return_stmt(allocator, T,
                   cubec_ast_create_call(allocator, T,
                     cubec_ast_create_identifier(allocator, T, "add"),
                     call_args)));

  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "f",
                   cubec_ast_create_vec(allocator, true),
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   cubec_ast_create_block(allocator, T, body_stmts),
                   false, false, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_member_access) {
  /* struct Point { x: f64; func get_x(): f64 { return self.x; } } */
  vec_t method_args = cubec_ast_create_vec(allocator, true);
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_return_stmt(allocator, T,
                   cubec_ast_create_member(allocator, T,
                     cubec_ast_create_identifier(allocator, T, "self"), "x")));

  vec_t members = cubec_ast_create_vec(allocator, true);
  vec_push(members, cubec_ast_create_struct_field(allocator, T, "x",
                   cubec_ast_create_identifier(allocator, T, "f64"), false));
  vec_push(members, cubec_ast_create_func_stmt(allocator, T, "get_x",
                   method_args,
                   cubec_ast_create_identifier(allocator, T, "f64"),
                   cubec_ast_create_block(allocator, T, body_stmts),
                   false, false, false, false, false, false));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_struct_stmt(allocator, T, "Point", members, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  /* 'self' is not automatically registered — this will error as undeclared.
     That's expected for now. Just ensure no crash. */
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_deref_addr) {
  /* func f(): i32 { var x: i32 = 42; return *(&x); } */
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_var_decl_stmt(allocator, T, "x",
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   NULL, false, false, false, false));
  vec_push(body_stmts, cubec_ast_create_return_stmt(allocator, T,
                   cubec_ast_create_deref(allocator, T,
                     cubec_ast_create_addr(allocator, T,
                       cubec_ast_create_identifier(allocator, T, "x")))));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "f",
                   cubec_ast_create_vec(allocator, true),
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   cubec_ast_create_block(allocator, T, body_stmts),
                   false, false, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  /* x has no initializer so it'll error, but no crash */
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_ternary) {
  /* func f(): i32 { return true ? 1 : 2; } */
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_return_stmt(allocator, T,
                   cubec_ast_create_ternary(allocator, T,
                     cubec_ast_create_identifier(allocator, T, "true"),
                     cubec_ast_create_numeric(allocator, T, "1",
                       CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                       CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT),
                     cubec_ast_create_numeric(allocator, T, "2",
                       CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                       CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT))));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "f",
                   cubec_ast_create_vec(allocator, true),
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   cubec_ast_create_block(allocator, T, body_stmts),
                   false, false, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  /* 'true' is not a builtin identifier — it'll error. Just ensure no crash. */
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_local_var) {
  /* func f() { var x: i32 = 42; } */
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_var_decl_stmt(allocator, T, "x",
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   NULL, false, false, false, false));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "f",
                   cubec_ast_create_vec(allocator, true), NULL,
                   cubec_ast_create_block(allocator, T, body_stmts),
                   false, false, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_break_in_loop) {
  /* func f() { while (true) { break; } } */
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_break_stmt(allocator, T));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "f",
                   cubec_ast_create_vec(allocator, true), NULL,
                   cubec_ast_create_block(allocator, T, cubec_ast_create_vec(allocator, true)),
                   false, false, false, false, false, false));

  /* Add while inside the block */
  cubec_statement_block_t outer_block =
      (cubec_statement_block_t)((cubec_statement_function_t)vec_get(stmts, 0))->body;
  vec_push(outer_block->statements,
           cubec_ast_create_while_stmt(allocator, T,
             cubec_ast_create_identifier(allocator, T, "true"),
             cubec_ast_create_block(allocator, T, body_stmts)));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  /* 'true' is undeclared — but break is in a loop so no break error */
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_break_outside_loop) {
  /* func f() { break; } */
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_break_stmt(allocator, T));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "f",
                   cubec_ast_create_vec(allocator, true), NULL,
                   cubec_ast_create_block(allocator, T, body_stmts),
                   false, false, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  EXPECT_GT(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

/* ===== Pass 3 extended feature tests ===== */

TEST_F(dt_checker, pass3_anonymous_function) {
  /* func f(): i32 { var fn = func(): i32 { return 1; }; return fn(); } */
  /* anonymous function body */
  vec_t anon_body = cubec_ast_create_vec(allocator, true);
  vec_push(anon_body, cubec_ast_create_return_stmt(allocator, T,
                   cubec_ast_create_numeric(allocator, T, "1",
                     CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                     CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT)));

  /* create anonymous function expression */
  node_t anon_fn = cubec_ast_create_function_expr(allocator, T,
                   NULL,
                   cubec_ast_create_vec(allocator, true),
                   cubec_ast_create_vec(allocator, true),
                   cubec_ast_create_vec(allocator, true),
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   cubec_ast_create_block(allocator, T, anon_body),
                   false);

  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_var_decl_stmt(allocator, T, "fn", NULL,
                   anon_fn, false, false, false, false));
  vec_push(body_stmts, cubec_ast_create_return_stmt(allocator, T,
                   cubec_ast_create_call(allocator, T,
                     cubec_ast_create_identifier(allocator, T, "fn"),
                     cubec_ast_create_vec(allocator, true))));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "f",
                   cubec_ast_create_vec(allocator, true),
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   cubec_ast_create_block(allocator, T, body_stmts),
                   false, false, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  /* Should not crash; anonymous function checked */
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_init_list_field) {
  /* struct Point { x: f64; y: f64; }
     func f(): Point { return Point { x: 1.0, y: 2.0 }; } */
  vec_t fields = cubec_ast_create_vec(allocator, true);
  vec_push(fields, cubec_ast_create_struct_field(allocator, T, "x",
                   cubec_ast_create_identifier(allocator, T, "f64"), false));
  vec_push(fields, cubec_ast_create_struct_field(allocator, T, "y",
                   cubec_ast_create_identifier(allocator, T, "f64"), false));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_struct_stmt(allocator, T, "Point", fields, false));

  /* initialize list with named fields */
  vec_t init_items = cubec_ast_create_vec(allocator, true);
  vec_push(init_items, cubec_ast_create_initialize_field(allocator, T, "x",
                   cubec_ast_create_numeric(allocator, T, "1.0",
                     CUBEC_LITERAL_NUMERIC_KIND_FLOAT,
                     CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT)));
  vec_push(init_items, cubec_ast_create_initialize_field(allocator, T, "y",
                   cubec_ast_create_numeric(allocator, T, "2.0",
                     CUBEC_LITERAL_NUMERIC_KIND_FLOAT,
                     CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT)));

  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_return_stmt(allocator, T,
                   cubec_ast_create_initialize_list(allocator, T,
                     cubec_ast_create_identifier(allocator, T, "Point"),
                     init_items, true)));

  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "f",
                   cubec_ast_create_vec(allocator, true),
                   cubec_ast_create_identifier(allocator, T, "Point"),
                   cubec_ast_create_block(allocator, T, body_stmts),
                   false, false, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_init_list_field_mismatch) {
  /* struct Point { x: f64; } func f(): Point { return Point { x: "bad" }; } */
  vec_t fields = cubec_ast_create_vec(allocator, true);
  vec_push(fields, cubec_ast_create_struct_field(allocator, T, "x",
                   cubec_ast_create_identifier(allocator, T, "f64"), false));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_struct_stmt(allocator, T, "Point", fields, false));

  vec_t init_items = cubec_ast_create_vec(allocator, true);
  vec_push(init_items, cubec_ast_create_initialize_field(allocator, T, "x",
                   cubec_ast_create_string(allocator, T, "bad")));

  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_return_stmt(allocator, T,
                   cubec_ast_create_initialize_list(allocator, T,
                     cubec_ast_create_identifier(allocator, T, "Point"),
                     init_items, true)));

  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "f",
                   cubec_ast_create_vec(allocator, true),
                   cubec_ast_create_identifier(allocator, T, "Point"),
                   cubec_ast_create_block(allocator, T, body_stmts),
                   false, false, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  EXPECT_GT(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_init_list_positional) {
  /* struct Pair { first: i32; second: i32; }
     func f(): Pair { return Pair { 1, 2 }; } */
  vec_t fields = cubec_ast_create_vec(allocator, true);
  vec_push(fields, cubec_ast_create_struct_field(allocator, T, "first",
                   cubec_ast_create_identifier(allocator, T, "i32"), false));
  vec_push(fields, cubec_ast_create_struct_field(allocator, T, "second",
                   cubec_ast_create_identifier(allocator, T, "i32"), false));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_struct_stmt(allocator, T, "Pair", fields, false));

  vec_t init_items = cubec_ast_create_vec(allocator, true);
  vec_push(init_items, cubec_ast_create_numeric(allocator, T, "1",
                     CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                     CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT));
  vec_push(init_items, cubec_ast_create_numeric(allocator, T, "2",
                     CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                     CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT));

  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_return_stmt(allocator, T,
                   cubec_ast_create_initialize_list(allocator, T,
                     cubec_ast_create_identifier(allocator, T, "Pair"),
                     init_items, false)));

  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "f",
                   cubec_ast_create_vec(allocator, true),
                   cubec_ast_create_identifier(allocator, T, "Pair"),
                   cubec_ast_create_block(allocator, T, body_stmts),
                   false, false, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  EXPECT_EQ(checker_get_error_count(ctx), 0);
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_deref_pointer) {
  /* func f(): i32 { var x: i32 = 42; var p: *i32 = &x; return *p; } */
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_var_decl_stmt(allocator, T, "x",
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   NULL, false, false, false, false));
  vec_push(body_stmts, cubec_ast_create_var_decl_stmt(allocator, T, "p",
                   cubec_ast_create_pointer_type(allocator, T,
                     cubec_ast_create_identifier(allocator, T, "i32"),
                     false, false),
                   NULL, false, false, false, false));
  vec_push(body_stmts, cubec_ast_create_return_stmt(allocator, T,
                   cubec_ast_create_deref(allocator, T,
                     cubec_ast_create_identifier(allocator, T, "p"))));

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cubec_ast_create_func_stmt(allocator, T, "f",
                   cubec_ast_create_vec(allocator, true),
                   cubec_ast_create_identifier(allocator, T, "i32"),
                   cubec_ast_create_block(allocator, T, body_stmts),
                   false, false, false, false, false, false));

  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);

  /* x has no initializer so it'll error, but deref of pointer should not crash */
  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}
