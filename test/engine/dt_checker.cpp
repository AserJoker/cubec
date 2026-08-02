#include "common/test_common.h"
#include "cubec/declaration_pointer.h"
#include "cubec/enum_item.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_call.h"
#include "cubec/expression_function.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/expression_member.h"
#include "cubec/expression_addr.h"
#include "cubec/expression_deref.h"
#include "cubec/expression_try.h"
#include "cubec/expression_ternary.h"
#include "cubec/function_argument.h"
#include "cubec/initialize_field.h"
#include "cubec/interface_method.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include "cubec/node.h"
#include "cubec/program.h"
#include "cubec/statement_block.h"
#include "cubec/statement_break.h"
#include "cubec/statement_cunion.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/statement_enum.h"
#include "cubec/statement_function.h"
#include "cubec/statement_interface.h"
#include "cubec/statement_return.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_union.h"
#include "cubec/statement_while.h"
#include "cubec/struct_field.h"
#include "cubec/union_field.h"
#include "engine/checker_collect.h"
#include "engine/checker_evaluate.h"
#include "engine/semantic_type.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include <gtest/gtest.h>


using ::testing::Test;

/* ===== helpers ===== */

static location_t test_loc() {
  static location_t loc = {
      .filename = "<test>", .begin = {1, 1, NULL}, .end = {1, 1, NULL}};
  return loc;
}

/* Shorthand aliases for brevity in tests */
#define T test_loc()

class dt_checker : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

TEST_F(dt_checker, create_and_dispose) {
  context_t checker = context_create(allocator);
  ASSERT_NE(checker, nullptr);
  EXPECT_NE(checker->global_scope, nullptr);
  EXPECT_NE(checker->diagnostics, nullptr);
  EXPECT_NE(checker->sources, nullptr);
  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
}

TEST_F(dt_checker, builtin_types_registered) {
  context_t checker = context_create(allocator);

  /* Check builtin types are non-null */
  EXPECT_NE(checker->builtin_void, nullptr);
  EXPECT_NE(checker->builtin_bool, nullptr);
  EXPECT_NE(checker->builtin_i32, nullptr);
  EXPECT_NE(checker->builtin_f64, nullptr);
  EXPECT_NE(checker->builtin_nil, nullptr);
  EXPECT_NE(checker->error_type, nullptr);

  /* Check kinds */
  EXPECT_EQ(semantic_type_get_kind(checker->builtin_void), TYPE_VOID);
  EXPECT_EQ(semantic_type_get_kind(checker->builtin_bool), TYPE_BOOL);
  EXPECT_EQ(semantic_type_get_kind(checker->builtin_i32), TYPE_I32);
  EXPECT_EQ(semantic_type_get_kind(checker->builtin_f64), TYPE_F64);
  EXPECT_EQ(semantic_type_get_kind(checker->builtin_str), TYPE_STR);
  EXPECT_EQ(semantic_type_get_kind(checker->builtin_nil), TYPE_NIL);
  EXPECT_EQ(semantic_type_get_kind(checker->error_type), TYPE_ERROR);

  context_dispose(checker);
}

TEST_F(dt_checker, builtin_types_in_scope) {
  context_t checker = context_create(allocator);

  /* Check that builtins are in global scope */
  struct symbol *sym = scope_lookup(checker->global_scope, "i32");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_TYPE);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);
  EXPECT_EQ(sym->type.type, checker->builtin_i32);

  sym = scope_lookup(checker->global_scope, "void");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_TYPE);
  EXPECT_EQ(sym->type.type, checker->builtin_void);

  context_dispose(checker);
}

TEST_F(dt_checker, builtin_types_in_name_table) {
  context_t checker = context_create(allocator);

  void *found = strmap_find(checker->type_name_table, "i32");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ((semantic_type_t)found, checker->builtin_i32);

  found = strmap_find(checker->type_name_table, "void");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ((semantic_type_t)found, checker->builtin_void);

  context_dispose(checker);
}

TEST_F(dt_checker, builtin_type_sizes) {
  context_t checker = context_create(allocator);

  EXPECT_EQ(semantic_type_get_size(checker->builtin_void), 0u);
  EXPECT_EQ(semantic_type_get_size(checker->builtin_bool), 1u);
  EXPECT_EQ(semantic_type_get_size(checker->builtin_i8), 1u);
  EXPECT_EQ(semantic_type_get_size(checker->builtin_i32), 4u);
  EXPECT_EQ(semantic_type_get_size(checker->builtin_i64), 8u);
  EXPECT_EQ(semantic_type_get_size(checker->builtin_f32), 4u);
  EXPECT_EQ(semantic_type_get_size(checker->builtin_f64), 8u);
  EXPECT_EQ(semantic_type_get_size(checker->builtin_str), 16u); /* ptr+len */

  context_dispose(checker);
}

/* ===== Pass 2 tests ===== */

TEST_F(dt_checker, pass2_struct_basic_debug) {
  /* Minimal: just create checker + empty struct */
  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts, create_statement_struct(ctx, T, "Point",
                                          create_vec(ctx, true), false, NULL, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  struct symbol *sym = scope_lookup(checker->global_scope, "Point");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_TYPE);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);

  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_struct_basic) {
  /* struct Point { x: f64; y: f64; } */
  vec_t members = create_vec(ctx, true);
  vec_push(members,
           create_struct_field(
               ctx, T, "x", create_literal_identifier(ctx, T, "f64"), false));
  vec_push(members,
           create_struct_field(
               ctx, T, "y", create_literal_identifier(ctx, T, "f64"), false));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_struct(ctx, T, "Point", members, false, NULL, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  /* Symbol should be EVALUATED */
  struct symbol *sym = scope_lookup(checker->global_scope, "Point");
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
  EXPECT_EQ(f0->field.type, checker->builtin_f64);
  EXPECT_EQ(f0->field.index, 0u);

  struct symbol *f1 = (struct symbol *)vec_get(fields, 1);
  EXPECT_STREQ(f1->name, "y");
  EXPECT_EQ(f1->field.type, checker->builtin_f64);
  EXPECT_EQ(f1->field.index, 1u);

  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_enum_basic) {
  /* enum Color { Red, Green, Blue } */
  vec_t items = create_vec(ctx, true);
  vec_push(items, create_enum_item(ctx, T, "Red", NULL, NULL));
  vec_push(items, create_enum_item(ctx, T, "Green", NULL, NULL));
  vec_push(items, create_enum_item(ctx, T, "Blue", NULL, NULL));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts, create_statement_enum(ctx, T, "Color", items, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  struct symbol *sym = scope_lookup(checker->global_scope, "Color");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_TYPE);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);

  semantic_type_t t = sym->type.type;
  ASSERT_NE(t, nullptr);
  EXPECT_FALSE(semantic_type_is_incomplete(t));
  EXPECT_EQ(semantic_type_get_kind(t), TYPE_ENUM);
  EXPECT_EQ(t->impl->enum_type.backing_type, checker->builtin_i32);
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

  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_union_basic) {
  /* union Value { int_val: i64; flt_val: f64; } */
  vec_t members = create_vec(ctx, true);
  vec_push(members,
           create_union_field(ctx, T, "int_val",
                              create_literal_identifier(ctx, T, "i64")));
  vec_push(members,
           create_union_field(ctx, T, "flt_val",
                              create_literal_identifier(ctx, T, "f64")));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_union(ctx, T, "Value", members, false, NULL, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  struct symbol *sym = scope_lookup(checker->global_scope, "Value");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);

  semantic_type_t t = sym->type.type;
  EXPECT_FALSE(semantic_type_is_incomplete(t));
  EXPECT_EQ(semantic_type_get_kind(t), TYPE_UNION);
  /* Tagged union: size = 8 (tag) + align_up(max(4,8), 8) = 16, alignment = 8 */
  EXPECT_EQ(semantic_type_get_size(t), 16u);
  EXPECT_EQ(semantic_type_get_alignment(t), 8u);

  vec_t fields = t->impl->struct_type.fields;
  ASSERT_EQ(vec_get_size(fields), 2u);

  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_cunion_basic) {
  /* cunion Data { a: i32; b: f64; } */
  vec_t fields = create_vec(ctx, true);
  vec_push(fields, create_struct_field(ctx, T, "a",
                                       create_literal_identifier(ctx, T, "i32"),
                                       false));
  vec_push(fields, create_struct_field(ctx, T, "b",
                                       create_literal_identifier(ctx, T, "f64"),
                                       false));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts, create_statement_cunion(ctx, T, "Data", fields, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  struct symbol *sym = scope_lookup(checker->global_scope, "Data");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);

  semantic_type_t t = sym->type.type;
  EXPECT_FALSE(semantic_type_is_incomplete(t));
  EXPECT_EQ(semantic_type_get_kind(t), TYPE_CUNION);
  /* cunion: size = max(4, 8) = 8, alignment = 8 */
  EXPECT_EQ(semantic_type_get_size(t), 8u);
  EXPECT_EQ(semantic_type_get_alignment(t), 8u);

  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_function_basic) {
  /* func add(a: i32, b: i32): i32 {} */
  vec_t args = create_vec(ctx, true);
  vec_push(args, create_function_argument(
                     ctx, T, "a", create_literal_identifier(ctx, T, "i32")));
  vec_push(args, create_function_argument(
                     ctx, T, "b", create_literal_identifier(ctx, T, "i32")));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_func(ctx, T, "add", args,
                                 create_literal_identifier(ctx, T, "i32"), NULL,
                                 false, false, false, false, false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  struct symbol *sym = scope_lookup(checker->global_scope, "add");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_FUNCTION);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);

  semantic_type_t ft = sym->function.type;
  ASSERT_NE(ft, nullptr);
  EXPECT_EQ(semantic_type_get_kind(ft), TYPE_FUNCTION);
  EXPECT_EQ(ft->impl->function.return_type, checker->builtin_i32);
  EXPECT_EQ(vec_get_size(ft->impl->function.params), 2u);

  /* Check param types */
  semantic_type_t p0 = (semantic_type_t)vec_get(ft->impl->function.params, 0);
  EXPECT_EQ(p0, checker->builtin_i32);
  semantic_type_t p1 = (semantic_type_t)vec_get(ft->impl->function.params, 1);
  EXPECT_EQ(p1, checker->builtin_i32);

  EXPECT_FALSE(ft->impl->function.is_variadic);
  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_variable_typed) {
  /* var x: i32 = 42 */
  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts, create_statement_declaration(
                      ctx, T, "x", create_literal_identifier(ctx, T, "i32"),
                      create_literal_numeric(
                          ctx, T, "42", CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                          CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT),
                      false, false, false, false, false));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  struct symbol *sym = scope_lookup(checker->global_scope, "x");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_VARIABLE);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);
  EXPECT_EQ(sym->variable.type, checker->builtin_i32);
  EXPECT_TRUE(sym->variable.is_mutable);

  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_variable_inferred_literal) {
  /* var n = 42 鈥?type inferred as i32 */
  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts, create_statement_declaration(
                      ctx, T, "n", NULL,
                      create_literal_numeric(
                          ctx, T, "42", CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                          CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT),
                      false, false, false, false, false));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  struct symbol *sym = scope_lookup(checker->global_scope, "n");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);
  EXPECT_EQ(sym->variable.type, checker->builtin_i32);

  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_type_alias) {
  /* type MyInt = i32 */
  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts, create_statement_declaration_type(
                      ctx, T, "MyInt", create_literal_identifier(ctx, T, "i32"),
                      false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  struct symbol *sym = scope_lookup(checker->global_scope, "MyInt");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->kind, SYMBOL_TYPE);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);
  EXPECT_EQ(sym->type.type, checker->builtin_i32);

  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_duplicate_continues) {
  /* Two structs with same name 鈥?Pass 1 errors, Pass 2 skips */
  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts, create_statement_struct(ctx, T, "Foo", create_vec(ctx, true),
                                          false, NULL, NULL));
  vec_push(stmts, create_statement_struct(ctx, T, "Foo", create_vec(ctx, true),
                                          false, NULL, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  /* First Foo should be resolved, second should error in Pass 1 */
  EXPECT_EQ(context_get_error_count(checker), 1);

  /* First Foo should still be EVALUATED */
  struct symbol *sym = scope_lookup(checker->global_scope, "Foo");
  ASSERT_NE(sym, nullptr);
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);

  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_incomplete_type_in_var) {
  /* Use void as value type 鈥?void is incomplete, should produce an error */
  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts, create_statement_declaration(
                      ctx, T, "x", create_literal_identifier(ctx, T, "void"),
                      NULL, false, false, false, false, false));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  /* Should have an error for incomplete type */
  EXPECT_GT(context_get_error_count(checker), 0);

  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_function_with_custom_types) {
  /* struct Point { x: f64; y: f64; }
     func origin(): Point {} */
  vec_t members = create_vec(ctx, true);
  vec_push(members,
           create_struct_field(
               ctx, T, "x", create_literal_identifier(ctx, T, "f64"), false));
  vec_push(members,
           create_struct_field(
               ctx, T, "y", create_literal_identifier(ctx, T, "f64"), false));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_struct(ctx, T, "Point", members, false, NULL, NULL));
  vec_push(stmts, create_statement_func(
                      ctx, T, "origin", create_vec(ctx, true),
                      create_literal_identifier(ctx, T, "Point"), NULL, false,
                      false, false, false, false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  /* Check function uses resolved Point type */
  struct symbol *fn_sym = scope_lookup(checker->global_scope, "origin");
  ASSERT_NE(fn_sym, nullptr);
  EXPECT_EQ(fn_sym->state, SYMBOL_EVALUATED);

  semantic_type_t ft = fn_sym->function.type;
  ASSERT_NE(ft, nullptr);

  struct symbol *pt_sym = scope_lookup(checker->global_scope, "Point");
  ASSERT_NE(pt_sym, nullptr);
  EXPECT_EQ(ft->impl->function.return_type, pt_sym->type.type);

  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

/* ===== numeric suffix inference tests ===== */

TEST_F(dt_checker, pass2_numeric_suffix_i64) {
  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts, create_statement_declaration(
                      ctx, T, "x", NULL,
                      create_literal_numeric(ctx, T, "42",
                                             CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                             CUBEC_LITERAL_NUMERIC_TYPE_I64),
                      false, false, false, false, false));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  struct symbol *sym = scope_lookup(checker->global_scope, "x");
  ASSERT_NE(sym, nullptr);
  ASSERT_NE(sym->variable.type, nullptr);
  EXPECT_EQ(sym->variable.type->impl->kind, TYPE_I64);

  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_numeric_suffix_f32) {
  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts, create_statement_declaration(
                      ctx, T, "pi", NULL,
                      create_literal_numeric(ctx, T, "3.14",
                                             CUBEC_LITERAL_NUMERIC_KIND_FLOAT,
                                             CUBEC_LITERAL_NUMERIC_TYPE_F32),
                      false, false, false, false, false));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  struct symbol *sym = scope_lookup(checker->global_scope, "pi");
  ASSERT_NE(sym, nullptr);
  ASSERT_NE(sym->variable.type, nullptr);
  EXPECT_EQ(sym->variable.type->impl->kind, TYPE_F32);

  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_numeric_default_float) {
  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts, create_statement_declaration(
                      ctx, T, "d", NULL,
                      create_literal_numeric(
                          ctx, T, "2.0", CUBEC_LITERAL_NUMERIC_KIND_FLOAT,
                          CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT),
                      false, false, false, false, false));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  struct symbol *sym = scope_lookup(checker->global_scope, "d");
  ASSERT_NE(sym, nullptr);
  ASSERT_NE(sym->variable.type, nullptr);
  EXPECT_EQ(sym->variable.type->impl->kind, TYPE_F64);

  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_numeric_default_int) {
  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts, create_statement_declaration(
                      ctx, T, "n", NULL,
                      create_literal_numeric(
                          ctx, T, "7", CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                          CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT),
                      false, false, false, false, false));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  struct symbol *sym = scope_lookup(checker->global_scope, "n");
  ASSERT_NE(sym, nullptr);
  ASSERT_NE(sym->variable.type, nullptr);
  EXPECT_EQ(sym->variable.type->impl->kind, TYPE_I32);

  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_numeric_suffix_u8) {
  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts, create_statement_declaration(
                      ctx, T, "byte", NULL,
                      create_literal_numeric(ctx, T, "255",
                                             CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                             CUBEC_LITERAL_NUMERIC_TYPE_U8),
                      false, false, false, false, false));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  struct symbol *sym = scope_lookup(checker->global_scope, "byte");
  ASSERT_NE(sym, nullptr);
  ASSERT_NE(sym->variable.type, nullptr);
  EXPECT_EQ(sym->variable.type->impl->kind, TYPE_U8);

  context_dispose(checker);
  allocator_free(allocator, &prog);
}

/* ===== struct method and static field tests ===== */

TEST_F(dt_checker, pass2_struct_method) {
  /* struct Counter { val: i32; func get(): i32 {} } */
  vec_t members = create_vec(ctx, true);
  vec_push(members,
           create_struct_field(
               ctx, T, "val", create_literal_identifier(ctx, T, "i32"), false));
  vec_t args = create_vec(ctx, true);
  vec_push(members,
           create_statement_func(ctx, T, "get", args,
                                 create_literal_identifier(ctx, T, "i32"), NULL,
                                 false, false, false, false, false, false, NULL));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_struct(ctx, T, "Counter", members, false, NULL, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  struct symbol *sym = scope_lookup(checker->global_scope, "Counter");
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

  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_struct_static_field) {
  /* struct Config { var count: i32; } */
  vec_t members = create_vec(ctx, true);
  vec_push(members,
           create_statement_declaration(
               ctx, T, "count", create_literal_identifier(ctx, T, "i32"), NULL,
               false, false, false, false, false));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_struct(ctx, T, "Config", members, false, NULL, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  struct symbol *sym = scope_lookup(checker->global_scope, "Config");
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

  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass2_interface_associated_type) {
  /* interface Hashable { type Key; func hash(key: Key): u64; } */
  vec_t members = create_vec(ctx, true);
  vec_push(members, create_statement_declaration_type(ctx, T, "Key", NULL,
                                                      false, false, NULL));
  vec_t args = create_vec(ctx, true);
  vec_push(args, create_function_argument(
                     ctx, T, "key", create_literal_identifier(ctx, T, "Key")));
  vec_push(members,
           create_interface_method(ctx, T, "hash", args,
                                   create_literal_identifier(ctx, T, "u64")));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_interface(ctx, T, "Hashable", members, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  struct symbol *sym = scope_lookup(checker->global_scope, "Hashable");
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

  context_dispose(checker);
  allocator_free(allocator, &prog);
}

/* ===== Pass 3 tests ===== */

TEST_F(dt_checker, pass3_return_typed) {
  /* func f(): i32 { return 42; } */
  vec_t body_stmts = create_vec(ctx, true);
  vec_push(body_stmts, create_statement_return(
                           ctx, T,
                           create_literal_numeric(
                               ctx, T, "42", CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                               CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT)));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_func(ctx, T, "f", create_vec(ctx, true),
                                 create_literal_identifier(ctx, T, "i32"),
                                 create_statement_block(ctx, T, body_stmts),
                                 false, false, false, false, false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_return_mismatch) {
  /* func f(): i32 { return "hello"; } 鈥?type mismatch */
  vec_t body_stmts = create_vec(ctx, true);
  vec_push(body_stmts, create_statement_return(
                           ctx, T, create_literal_string(ctx, T, "hello")));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_func(ctx, T, "f", create_vec(ctx, true),
                                 create_literal_identifier(ctx, T, "i32"),
                                 create_statement_block(ctx, T, body_stmts),
                                 false, false, false, false, false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  EXPECT_GT(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_return_void) {
  /* func f() { return; } 鈥?bare return in void function */
  vec_t body_stmts = create_vec(ctx, true);
  vec_push(body_stmts, create_statement_return(ctx, T, NULL));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_func(ctx, T, "f", create_vec(ctx, true), NULL,
                                 create_statement_block(ctx, T, body_stmts),
                                 false, false, false, false, false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_binary_arithmetic) {
  /* func f(): i32 { return 1 + 2; } */
  vec_t body_stmts = create_vec(ctx, true);
  vec_push(body_stmts,
           create_statement_return(
               ctx, T,
               create_expression_binary(
                   ctx, T, "+",
                   create_literal_numeric(ctx, T, "1",
                                          CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                          CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT),
                   create_literal_numeric(
                       ctx, T, "2", CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                       CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT))));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_func(ctx, T, "f", create_vec(ctx, true),
                                 create_literal_identifier(ctx, T, "i32"),
                                 create_statement_block(ctx, T, body_stmts),
                                 false, false, false, false, false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_binary_comparison) {
  /* func f(): bool { return 1 < 2; } */
  vec_t body_stmts = create_vec(ctx, true);
  vec_push(body_stmts,
           create_statement_return(
               ctx, T,
               create_expression_binary(
                   ctx, T, "<",
                   create_literal_numeric(ctx, T, "1",
                                          CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                          CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT),
                   create_literal_numeric(
                       ctx, T, "2", CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                       CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT))));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_func(ctx, T, "f", create_vec(ctx, true),
                                 create_literal_identifier(ctx, T, "bool"),
                                 create_statement_block(ctx, T, body_stmts),
                                 false, false, false, false, false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_call_basic) {
  /* func add(a: i32, b: i32): i32 {} func f(): i32 { return add(1, 2); } */
  vec_t add_args = create_vec(ctx, true);
  vec_push(add_args,
           create_function_argument(ctx, T, "a",
                                    create_literal_identifier(ctx, T, "i32")));
  vec_push(add_args,
           create_function_argument(ctx, T, "b",
                                    create_literal_identifier(ctx, T, "i32")));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_func(ctx, T, "add", add_args,
                                 create_literal_identifier(ctx, T, "i32"), NULL,
                                 false, false, false, false, false, false, NULL));

  vec_t call_args = create_vec(ctx, true);
  vec_push(call_args, create_literal_numeric(
                          ctx, T, "1", CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                          CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT));
  vec_push(call_args, create_literal_numeric(
                          ctx, T, "2", CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                          CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT));

  vec_t body_stmts = create_vec(ctx, true);
  vec_push(body_stmts, create_statement_return(
                           ctx, T,
                           create_expression_call(
                               ctx, T, create_literal_identifier(ctx, T, "add"),
                               call_args)));

  vec_push(stmts,
           create_statement_func(ctx, T, "f", create_vec(ctx, true),
                                 create_literal_identifier(ctx, T, "i32"),
                                 create_statement_block(ctx, T, body_stmts),
                                 false, false, false, false, false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_member_access) {
  /* struct Point { x: f64; func get_x(): f64 { return self.x; } } */
  vec_t method_args = create_vec(ctx, true);
  vec_t body_stmts = create_vec(ctx, true);
  vec_push(body_stmts,
           create_statement_return(
               ctx, T,
               create_expression_member(
                   ctx, T, create_literal_identifier(ctx, T, "self"), "x")));

  vec_t members = create_vec(ctx, true);
  vec_push(members,
           create_struct_field(
               ctx, T, "x", create_literal_identifier(ctx, T, "f64"), false));
  vec_push(members,
           create_statement_func(ctx, T, "get_x", method_args,
                                 create_literal_identifier(ctx, T, "f64"),
                                 create_statement_block(ctx, T, body_stmts),
                                 false, false, false, false, false, false, NULL));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_struct(ctx, T, "Point", members, false, NULL, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  /* 'self' is not automatically registered 鈥?this will error as undeclared.
     That's expected for now. Just ensure no crash. */
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_deref_addr) {
  /* func f(): i32 { var x: i32 = 42; return *(&x); } */
  vec_t body_stmts = create_vec(ctx, true);
  vec_push(body_stmts,
           create_statement_declaration(
               ctx, T, "x", create_literal_identifier(ctx, T, "i32"), NULL,
               false, false, false, false, false));
  vec_push(body_stmts,
           create_statement_return(
               ctx, T,
              create_expression_deref(
                  ctx, T,
                  create_expression_addr(
                      ctx, T, create_literal_identifier(ctx, T, "x")))));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_func(ctx, T, "f", create_vec(ctx, true),
                                 create_literal_identifier(ctx, T, "i32"),
                                 create_statement_block(ctx, T, body_stmts),
                                 false, false, false, false, false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  /* x has no initializer so it'll error, but no crash */
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_ternary) {
  /* func f(): i32 { return true ? 1 : 2; } */
  vec_t body_stmts = create_vec(ctx, true);
  vec_push(body_stmts,
           create_statement_return(
               ctx, T,
               create_expression_ternary(
                   ctx, T, create_literal_identifier(ctx, T, "true"),
                   create_literal_numeric(ctx, T, "1",
                                          CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                          CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT),
                   create_literal_numeric(
                       ctx, T, "2", CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                       CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT))));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_func(ctx, T, "f", create_vec(ctx, true),
                                 create_literal_identifier(ctx, T, "i32"),
                                 create_statement_block(ctx, T, body_stmts),
                                 false, false, false, false, false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  /* 'true' is not a builtin identifier 鈥?it'll error. Just ensure no crash. */
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_local_var) {
  /* func f() { var x: i32 = 42; } */
  vec_t body_stmts = create_vec(ctx, true);
  vec_push(body_stmts,
           create_statement_declaration(
               ctx, T, "x", create_literal_identifier(ctx, T, "i32"),
               create_literal_numeric(ctx, T, "42",
                                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                      CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT),
               false, false, false, false, false));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_func(ctx, T, "f", create_vec(ctx, true), NULL,
                                 create_statement_block(ctx, T, body_stmts),
                                 false, false, false, false, false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_break_in_loop) {
  /* func f() { while (true) { break; } } */
  vec_t body_stmts = create_vec(ctx, true);
  vec_push(body_stmts, create_statement_break(ctx, T));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts, create_statement_func(
                      ctx, T, "f", create_vec(ctx, true), NULL,
                      create_statement_block(ctx, T, create_vec(ctx, true)),
                      false, false, false, false, false, false, NULL));

  /* Add while inside the block */
  cubec_statement_block_t outer_block =
      (cubec_statement_block_t)((cubec_statement_function_t)vec_get(stmts, 0))
          ->body;
  vec_push(outer_block->statements,
           create_create_while(ctx, T,
                               create_literal_identifier(ctx, T, "true"),
                               create_statement_block(ctx, T, body_stmts)));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  /* 'true' is undeclared 鈥?but break is in a loop so no break error */
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_break_outside_loop) {
  /* func f() { break; } */
  vec_t body_stmts = create_vec(ctx, true);
  vec_push(body_stmts, create_statement_break(ctx, T));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_func(ctx, T, "f", create_vec(ctx, true), NULL,
                                 create_statement_block(ctx, T, body_stmts),
                                 false, false, false, false, false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  EXPECT_GT(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

/* ===== Pass 3 extended feature tests ===== */

TEST_F(dt_checker, pass3_anonymous_function) {
  /* func f(): i32 { var fn = func(): i32 { return 1; }; return fn(); } */
  /* anonymous function body */
  vec_t anon_body = create_vec(ctx, true);
  vec_push(anon_body, create_statement_return(
                          ctx, T,
                          create_literal_numeric(
                              ctx, T, "1", CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                              CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT)));

  /* create anonymous function expression */
  node_t anon_fn = create_expression_function(
      ctx, T, NULL, create_vec(ctx, true), create_vec(ctx, true),
      create_vec(ctx, true), create_literal_identifier(ctx, T, "i32"),
      create_statement_block(ctx, T, anon_body), false);

  vec_t body_stmts = create_vec(ctx, true);
  vec_push(body_stmts,
           create_statement_declaration(ctx, T, "fn", NULL, anon_fn, false,
                                        false, false, false, false));
  vec_push(body_stmts, create_statement_return(
                           ctx, T,
                           create_expression_call(
                               ctx, T, create_literal_identifier(ctx, T, "fn"),
                               create_vec(ctx, true))));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_func(ctx, T, "f", create_vec(ctx, true),
                                 create_literal_identifier(ctx, T, "i32"),
                                 create_statement_block(ctx, T, body_stmts),
                                 false, false, false, false, false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  /* Should not crash; anonymous function checked */
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_init_list_field) {
  /* struct Point { x: f64; y: f64; }
     func f(): Point { return Point { x: 1.0, y: 2.0 }; } */
  vec_t fields = create_vec(ctx, true);
  vec_push(fields, create_struct_field(ctx, T, "x",
                                       create_literal_identifier(ctx, T, "f64"),
                                       false));
  vec_push(fields, create_struct_field(ctx, T, "y",
                                       create_literal_identifier(ctx, T, "f64"),
                                       false));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_struct(ctx, T, "Point", fields, false, NULL, NULL));

  /* initialize list with named fields */
  vec_t init_items = create_vec(ctx, true);
  vec_push(init_items, create_initialize_field(
                           ctx, T, "x",
                           create_literal_numeric(
                               ctx, T, "1.0", CUBEC_LITERAL_NUMERIC_KIND_FLOAT,
                               CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT)));
  vec_push(init_items, create_initialize_field(
                           ctx, T, "y",
                           create_literal_numeric(
                               ctx, T, "2.0", CUBEC_LITERAL_NUMERIC_KIND_FLOAT,
                               CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT)));

  vec_t body_stmts = create_vec(ctx, true);
  vec_push(body_stmts,
           create_statement_return(
               ctx, T,
               create_expression_initialize_list(
                   ctx, T, create_literal_identifier(ctx, T, "Point"),
                   init_items, true)));

  vec_push(stmts,
           create_statement_func(ctx, T, "f", create_vec(ctx, true),
                                 create_literal_identifier(ctx, T, "Point"),
                                 create_statement_block(ctx, T, body_stmts),
                                 false, false, false, false, false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_init_list_field_mismatch) {
  /* struct Point { x: f64; } func f(): Point { return Point { x: "bad" }; } */
  vec_t fields = create_vec(ctx, true);
  vec_push(fields, create_struct_field(ctx, T, "x",
                                       create_literal_identifier(ctx, T, "f64"),
                                       false));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_struct(ctx, T, "Point", fields, false, NULL, NULL));

  vec_t init_items = create_vec(ctx, true);
  vec_push(init_items, create_initialize_field(
                           ctx, T, "x", create_literal_string(ctx, T, "bad")));

  vec_t body_stmts = create_vec(ctx, true);
  vec_push(body_stmts,
           create_statement_return(
               ctx, T,
               create_expression_initialize_list(
                   ctx, T, create_literal_identifier(ctx, T, "Point"),
                   init_items, true)));

  vec_push(stmts,
           create_statement_func(ctx, T, "f", create_vec(ctx, true),
                                 create_literal_identifier(ctx, T, "Point"),
                                 create_statement_block(ctx, T, body_stmts),
                                 false, false, false, false, false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  EXPECT_GT(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_init_list_positional) {
  /* struct Pair { first: i32; second: i32; }
     func f(): Pair { return Pair { 1, 2 }; } */
  vec_t fields = create_vec(ctx, true);
  vec_push(fields, create_struct_field(ctx, T, "first",
                                       create_literal_identifier(ctx, T, "i32"),
                                       false));
  vec_push(fields, create_struct_field(ctx, T, "second",
                                       create_literal_identifier(ctx, T, "i32"),
                                       false));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts, create_statement_struct(ctx, T, "Pair", fields, false, NULL, NULL));

  vec_t init_items = create_vec(ctx, true);
  vec_push(init_items, create_literal_numeric(
                           ctx, T, "1", CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                           CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT));
  vec_push(init_items, create_literal_numeric(
                           ctx, T, "2", CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                           CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT));

  vec_t body_stmts = create_vec(ctx, true);
  vec_push(body_stmts,
           create_statement_return(
               ctx, T,
               create_expression_initialize_list(
                   ctx, T, create_literal_identifier(ctx, T, "Pair"),
                   init_items, false)));

  vec_push(stmts,
           create_statement_func(ctx, T, "f", create_vec(ctx, true),
                                 create_literal_identifier(ctx, T, "Pair"),
                                 create_statement_block(ctx, T, body_stmts),
                                 false, false, false, false, false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  EXPECT_EQ(context_get_error_count(checker), 0);
  context_dispose(checker);
  allocator_free(allocator, &prog);
}

TEST_F(dt_checker, pass3_deref_pointer) {
  /* func f(): i32 { var x: i32 = 42; var p: *i32 = &x; return *p; } */
  vec_t body_stmts = create_vec(ctx, true);
  vec_push(body_stmts,
           create_statement_declaration(
               ctx, T, "x", create_literal_identifier(ctx, T, "i32"), NULL,
               false, false, false, false, false));
  vec_push(body_stmts, create_statement_declaration(
                           ctx, T, "p",
                           create_declaration_pointer(
                               ctx, T, create_literal_identifier(ctx, T, "i32"),
                               false, false),
                           NULL, false, false, false, false, false));
  vec_push(body_stmts,
           create_statement_return(
               ctx, T,
              create_expression_deref(
                  ctx, T, create_literal_identifier(ctx, T, "p"))));

  vec_t stmts = create_vec(ctx, true);
  vec_push(stmts,
           create_statement_func(ctx, T, "f", create_vec(ctx, true),
                                 create_literal_identifier(ctx, T, "i32"),
                                 create_statement_block(ctx, T, body_stmts),
                                 false, false, false, false, false, false, NULL));

  node_t prog = create_program(ctx, T, stmts);
  context_t checker = context_create(allocator);
  context_check_program(checker, prog);

  /* x has no initializer so it'll error, but deref of pointer should not crash
   */
  context_dispose(checker);
  allocator_free(allocator, &prog);
}
