#include "engine/comptime_eval.h"
#include "engine/comptime_value.h"
#include "engine/checker_evaluate.h"
#include "engine/checker_collect.h"
#include "engine/checker_evaluate.h"
#include "engine/symbol.h"
#include "engine/type_layout.h"
#include "cubec/ast_factory.h"
#include "cubec/literal_numeric.h"
#include "cubec/statement.h"
#include "cubec/statement_block.h"
#include "cubec/statement_comptime.h"
#include "cubec/statement_return.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include <stdarg.h>

using ::testing::Test;

/* ===== helpers ===== */

static location_t test_loc() {
  static location_t loc = {.filename = "<test>",
                            .begin = {1, 1, NULL},
                            .end = {1, 1, NULL}};
  return loc;
}

#define T test_loc()

/** Create AST node for: builtin func assert(condition: bool): void; */
static node_t create_builtin_assert(context_t ctx) {
  vec_t args = cubec_ast_create_vec(ctx, true);
  node_t param = cubec_ast_create_func_arg(ctx, T, "condition",
      cubec_ast_create_identifier(ctx, T, "bool"));
  vec_push(args, param);
  return cubec_ast_create_func_stmt(ctx, T, "assert", args,
      cubec_ast_create_identifier(ctx, T, "void"), NULL,
      false, false, false, true, false, false);
}

class dt_comptime_eval : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ===== literal evaluation ===== */

TEST_F(dt_comptime_eval, eval_int_literal) {
  context_t checker = context_create(allocator);
  node_t num = cubec_ast_create_numeric(checker, T, "42",
                                          CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                          CUBEC_LITERAL_NUMERIC_TYPE_I32);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, num);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(v->int_val.s, 42);
  allocator_free(allocator, &num);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, eval_bool_literal) {
  context_t checker = context_create(allocator);
  /* true = identifier "true" */
  node_t b = cubec_ast_create_identifier(checker, T, "true");
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, b);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_BOOL);
  EXPECT_TRUE(v->bool_val);
  allocator_free(allocator, &b);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, eval_string_literal) {
  context_t checker = context_create(allocator);
  node_t s = cubec_ast_create_string(checker, T, "hello");
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, s);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_STRING);
  EXPECT_STREQ(comptime_value_get_string(v), "hello");
  allocator_free(allocator, &s);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, eval_char_literal) {
  context_t checker = context_create(allocator);
  node_t c = cubec_ast_create_char(checker, T, 'X');
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, c);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_CHAR);
  EXPECT_EQ(v->char_val, 'X');
  allocator_free(allocator, &c);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, eval_nil_literal) {
  context_t checker = context_create(allocator);
  node_t n = cubec_ast_create_identifier(checker, T, "nil");
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, n);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_NIL);
  allocator_free(allocator, &n);
  context_dispose(checker);
}

/* ===== arithmetic ===== */

TEST_F(dt_comptime_eval, eval_add) {
  context_t checker = context_create(allocator);
  node_t left = cubec_ast_create_numeric(checker, T, "10",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(checker, T, "20",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(checker, T, "+", left, right);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(v->int_val.s, 30);
  allocator_free(allocator, &bin);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, eval_sub) {
  context_t checker = context_create(allocator);
  node_t left = cubec_ast_create_numeric(checker, T, "50",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(checker, T, "20",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(checker, T, "-", left, right);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 30);
  allocator_free(allocator, &bin);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, eval_mul) {
  context_t checker = context_create(allocator);
  node_t left = cubec_ast_create_numeric(checker, T, "6",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(checker, T, "7",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(checker, T, "*", left, right);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 42);
  allocator_free(allocator, &bin);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, eval_float_add) {
  context_t checker = context_create(allocator);
  node_t left = cubec_ast_create_numeric(checker, T, "1.5",
                                           CUBEC_LITERAL_NUMERIC_KIND_FLOAT,
                                           CUBEC_LITERAL_NUMERIC_TYPE_F64);
  node_t right = cubec_ast_create_numeric(checker, T, "2.5",
                                            CUBEC_LITERAL_NUMERIC_KIND_FLOAT,
                                            CUBEC_LITERAL_NUMERIC_TYPE_F64);
  node_t bin = cubec_ast_create_binary(checker, T, "+", left, right);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_FLOAT);
  EXPECT_DOUBLE_EQ(v->float_val.value, 4.0);
  allocator_free(allocator, &bin);
  context_dispose(checker);
}

/* ===== comparison ===== */

TEST_F(dt_comptime_eval, eval_eq) {
  context_t checker = context_create(allocator);
  node_t left = cubec_ast_create_numeric(checker, T, "5",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(checker, T, "5",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(checker, T, "==", left, right);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_BOOL);
  EXPECT_TRUE(v->bool_val);
  allocator_free(allocator, &bin);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, eval_neq) {
  context_t checker = context_create(allocator);
  node_t left = cubec_ast_create_numeric(checker, T, "5",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(checker, T, "3",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(checker, T, "!=", left, right);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(v->bool_val);
  allocator_free(allocator, &bin);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, eval_lt) {
  context_t checker = context_create(allocator);
  node_t left = cubec_ast_create_numeric(checker, T, "3",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(checker, T, "5",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(checker, T, "<", left, right);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(v->bool_val);
  allocator_free(allocator, &bin);
  context_dispose(checker);
}

/* ===== unary ===== */

TEST_F(dt_comptime_eval, eval_negate) {
  context_t checker = context_create(allocator);
  node_t right = cubec_ast_create_numeric(checker, T, "7",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(checker, T, "-", NULL, right);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(v->int_val.s, -7);
  allocator_free(allocator, &bin);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, eval_logical_not) {
  context_t checker = context_create(allocator);
  node_t right = cubec_ast_create_identifier(checker, T, "true");
  node_t bin = cubec_ast_create_binary(checker, T, "!", NULL, right);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_BOOL);
  EXPECT_FALSE(v->bool_val);
  allocator_free(allocator, &bin);
  context_dispose(checker);
}

/* ===== logical binary ===== */

TEST_F(dt_comptime_eval, eval_logical_and) {
  context_t checker = context_create(allocator);
  node_t left = cubec_ast_create_identifier(checker, T, "true");
  node_t right = cubec_ast_create_identifier(checker, T, "false");
  node_t bin = cubec_ast_create_binary(checker, T, "&&", left, right);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_BOOL);
  EXPECT_FALSE(v->bool_val);
  allocator_free(allocator, &bin);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, eval_logical_or) {
  context_t checker = context_create(allocator);
  node_t left = cubec_ast_create_identifier(checker, T, "false");
  node_t right = cubec_ast_create_identifier(checker, T, "true");
  node_t bin = cubec_ast_create_binary(checker, T, "||", left, right);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(v->bool_val);
  allocator_free(allocator, &bin);
  context_dispose(checker);
}

/* ===== ternary ===== */

TEST_F(dt_comptime_eval, eval_ternary_true) {
  context_t checker = context_create(allocator);
  node_t cond = cubec_ast_create_identifier(checker, T, "true");
  node_t then_b = cubec_ast_create_numeric(checker, T, "10",
                                              CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                              CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t else_b = cubec_ast_create_numeric(checker, T, "20",
                                              CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                              CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t tern = cubec_ast_create_ternary(checker, T, cond, then_b, else_b);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, tern);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 10);
  allocator_free(allocator, &tern);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, eval_ternary_false) {
  context_t checker = context_create(allocator);
  node_t cond = cubec_ast_create_identifier(checker, T, "false");
  node_t then_b = cubec_ast_create_numeric(checker, T, "10",
                                              CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                              CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t else_b = cubec_ast_create_numeric(checker, T, "20",
                                              CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                              CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t tern = cubec_ast_create_ternary(checker, T, cond, then_b, else_b);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, tern);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 20);
  allocator_free(allocator, &tern);
  context_dispose(checker);
}

/* ===== typeof / sizeof / alignof ===== */

TEST_F(dt_comptime_eval, eval_typeof) {
  context_t checker = context_create(allocator);
  node_t inner = cubec_ast_create_identifier(checker, T, "i32");
  node_t tof = cubec_ast_create_typeof(checker, T, inner);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, tof);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_TYPE);
  EXPECT_EQ(v->type_val, checker->builtin_i32);
  allocator_free(allocator, &tof);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, eval_sizeof) {
  context_t checker = context_create(allocator);
  node_t inner = cubec_ast_create_identifier(checker, T, "i32");
  node_t sof = cubec_ast_create_sizeof(checker, T, inner);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, sof);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(v->int_val.s, 4);
  allocator_free(allocator, &sof);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, eval_alignof) {
  context_t checker = context_create(allocator);
  node_t inner = cubec_ast_create_identifier(checker, T, "i32");
  node_t aof = cubec_ast_create_alignof(checker, T, inner);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, aof);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT);
  EXPECT_GE(v->int_val.s, 1);
  allocator_free(allocator, &aof);
  context_dispose(checker);
}

/* ===== variable declaration & access ===== */

TEST_F(dt_comptime_eval, comptime_var_decl_and_use) {
  context_t checker = context_create(allocator);
  /* comptime x: i32 = 42; */
  node_t type_id = cubec_ast_create_identifier(checker, T, "i32");
  node_t init = cubec_ast_create_numeric(checker, T, "42",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t var = cubec_ast_create_var_decl_stmt(checker, T, "x",
                    type_id, init, false, false, false, true, false);
  vec_t stmts = cubec_ast_create_vec(checker, true);
  vec_push(stmts, var);
  node_t prog = cubec_ast_create_program(checker, T, stmts);
  context_check_program(checker, prog);

  /* The comptime var should have been evaluated and bound in the env */
  comptime_value_t v = comptime_env_lookup_value(checker->comptime_eval->global_env, checker->comptime_eval->valloc, "x");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(v->int_val.s, 42);

  allocator_free(allocator, &prog);
  context_dispose(checker);
}

/* ===== if statement ===== */

TEST_F(dt_comptime_eval, comptime_if_true) {
  context_t checker = context_create(allocator);
  /* comptime if true { ... } */
  vec_t empty_vec = cubec_ast_create_vec(checker, true);
  node_t then_body = cubec_ast_create_block(checker, T, empty_vec);
  node_t cond = cubec_ast_create_identifier(checker, T, "true");
  node_t cif = cubec_ast_create_if_stmt(checker, T, cond, then_body, NULL);
  comptime_signal_t sig = comptime_eval_exec_comptime_if(checker->comptime_eval, checker, cif);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);
  allocator_free(allocator, &cif);
  context_dispose(checker);
}

/* ===== for statement ===== */

TEST_F(dt_comptime_eval, comptime_for_loop) {
  context_t checker = context_create(allocator);

  /* Build: comptime for init=var sum:i32=0, cond=sum<3, incr=sum=sum+1, body=empty */
  node_t type_id = cubec_ast_create_identifier(checker, T, "i32");
  node_t init_val = cubec_ast_create_numeric(checker, T, "0",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t init = cubec_ast_create_var_decl_stmt(checker, T, "sum",
                  type_id, init_val, false, false, false, true, false);

  node_t sum_id = cubec_ast_create_identifier(checker, T, "sum");
  node_t three = cubec_ast_create_numeric(checker, T, "3",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t cond = cubec_ast_create_binary(checker, T, "<", sum_id, three);

  node_t one = cubec_ast_create_numeric(checker, T, "1",
                  CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                  CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t sum_id2 = cubec_ast_create_identifier(checker, T, "sum");
  node_t sum_id3 = cubec_ast_create_identifier(checker, T, "sum");
  node_t sum_plus_1 = cubec_ast_create_binary(checker, T, "+", sum_id2, one);
  node_t incr = cubec_ast_create_assignment(checker, T, "=", sum_id3, sum_plus_1);

  vec_t empty_vec = cubec_ast_create_vec(checker, true);
  node_t body = cubec_ast_create_block(checker, T, empty_vec);

  node_t for_stmt = cubec_ast_create_for_stmt(checker, T, init, cond, incr, body);

  comptime_signal_t sig = comptime_eval_exec_stmt(checker->comptime_eval, checker, for_stmt);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);

  allocator_free(allocator, &for_stmt);
  context_dispose(checker);
}

/* ===== function call ===== */

TEST_F(dt_comptime_eval, function_call) {
  context_t checker = context_create(allocator);

  /* Build: func double(x: i32): i32 { return x + x; } */
  node_t x_type = cubec_ast_create_identifier(checker, T, "i32");
  vec_t args = cubec_ast_create_vec(checker, true);
  vec_push(args, cubec_ast_create_func_arg(checker, T, "x", x_type));

  node_t x_id = cubec_ast_create_identifier(checker, T, "x");
  node_t x_id2 = cubec_ast_create_identifier(checker, T, "x");
  node_t x_plus_x = cubec_ast_create_binary(checker, T, "+", x_id, x_id2);
  node_t ret_stmt = cubec_ast_create_return_stmt(checker, T, x_plus_x);
  vec_t body_stmts = cubec_ast_create_vec(checker, true);
  vec_push(body_stmts, ret_stmt);
  node_t body = cubec_ast_create_block(checker, T, body_stmts);

  node_t ret_type = cubec_ast_create_identifier(checker, T, "i32");
  node_t fn = cubec_ast_create_func_stmt(checker, T, "double", args,
                ret_type, body, false, false, false, false, false, false);

  vec_t stmts = cubec_ast_create_vec(checker, true);
  vec_push(stmts, fn);
  node_t prog = cubec_ast_create_program(checker, T, stmts);
  context_check_program(checker, prog);

  /* Verify function is in comptime env */
  comptime_value_t fn_val = comptime_env_lookup_value(checker->comptime_eval->global_env, checker->comptime_eval->valloc, "double");
  ASSERT_NE(fn_val, nullptr) << "function 'double' not in comptime env";
  EXPECT_EQ(fn_val->kind, COMPTIME_VALUE_FUNCTION) << "got kind=" << fn_val->kind;

  /* Now call double(21) via the evaluator */
  node_t arg21 = cubec_ast_create_numeric(checker, T, "21",
                        CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                        CUBEC_LITERAL_NUMERIC_TYPE_I32);
  vec_t call_args = cubec_ast_create_vec(checker, true);
  vec_push(call_args, arg21);
  node_t callee = cubec_ast_create_identifier(checker, T, "double");
  node_t call = cubec_ast_create_call(checker, T, callee, call_args);

  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, call);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT) << "got kind=" << v->kind;
  EXPECT_EQ(v->int_val.s, 42);

  allocator_free(allocator, &call);
  allocator_free(allocator, &prog);
  context_dispose(checker);
}

/* ===== comptime block ===== */

TEST_F(dt_comptime_eval, comptime_block_executes) {
  context_t checker = context_create(allocator);

  /* comptime { } — empty comptime block */
  vec_t empty_vec = cubec_ast_create_vec(checker, true);
  node_t block = cubec_ast_create_block(checker, T, empty_vec);
  comptime_signal_t sig = comptime_eval_exec_block(checker->comptime_eval, checker, block);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);
  allocator_free(allocator, &block);
  context_dispose(checker);
}

/* ===== group expression ===== */

TEST_F(dt_comptime_eval, eval_group) {
  context_t checker = context_create(allocator);
  node_t inner = cubec_ast_create_numeric(checker, T, "42",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t grp = cubec_ast_create_group(checker, T, inner);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, grp);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 42);
  allocator_free(allocator, &grp);
  context_dispose(checker);
}

/* ===== comma expression ===== */

TEST_F(dt_comptime_eval, eval_comma) {
  context_t checker = context_create(allocator);
  node_t left = cubec_ast_create_numeric(checker, T, "1",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(checker, T, "2",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t comma = cubec_ast_create_comma(checker, T, left, right);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, comma);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 2); /* comma returns right */
  allocator_free(allocator, &comma);
  context_dispose(checker);
}

/* ===== bitwise ===== */

TEST_F(dt_comptime_eval, eval_bitwise_and) {
  context_t checker = context_create(allocator);
  node_t left = cubec_ast_create_numeric(checker, T, "12",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(checker, T, "10",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(checker, T, "&", left, right);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT) << "got kind=" << v->kind;
  if (v->kind == COMPTIME_VALUE_INT)
    EXPECT_EQ(v->int_val.s, 8); /* 12 & 10 = 8 */
  allocator_free(allocator, &bin);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, eval_bitwise_or) {
  context_t checker = context_create(allocator);
  node_t left = cubec_ast_create_numeric(checker, T, "12",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(checker, T, "10",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(checker, T, "|", left, right);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 14); /* 12 | 10 = 14 */
  allocator_free(allocator, &bin);
  context_dispose(checker);
}

/* ===== break / continue in loops ===== */

TEST_F(dt_comptime_eval, break_in_for) {
  context_t checker = context_create(allocator);

  /* for init: x:i32=0, cond: true, incr: none, body: { break } */
  node_t type_id = cubec_ast_create_identifier(checker, T, "i32");
  node_t init_val = cubec_ast_create_numeric(checker, T, "0",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t init = cubec_ast_create_var_decl_stmt(checker, T, "x",
                  type_id, init_val, false, false, false, false, false);

  node_t break_stmt = cubec_ast_create_break_stmt(checker, T);
  vec_t body_stmts = cubec_ast_create_vec(checker, true);
  vec_push(body_stmts, break_stmt);
  node_t body = cubec_ast_create_block(checker, T, body_stmts);

  node_t true_id = cubec_ast_create_identifier(checker, T, "true");
  node_t for_stmt = cubec_ast_create_for_stmt(checker, T, init,
                    true_id, NULL, body);

  comptime_signal_t sig = comptime_eval_exec_stmt(checker->comptime_eval, checker, for_stmt);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);
  allocator_free(allocator, &for_stmt);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, continue_in_for) {
  context_t checker = context_create(allocator);

  /* for init: x:i32=0, cond: x<3, incr: x=x+1, body: { continue } */
  node_t type_id = cubec_ast_create_identifier(checker, T, "i32");
  node_t init_val = cubec_ast_create_numeric(checker, T, "0",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t init = cubec_ast_create_var_decl_stmt(checker, T, "x",
                  type_id, init_val, false, false, false, false, false);

  node_t x_id = cubec_ast_create_identifier(checker, T, "x");
  node_t three = cubec_ast_create_numeric(checker, T, "3",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t cond = cubec_ast_create_binary(checker, T, "<", x_id, three);

  node_t one = cubec_ast_create_numeric(checker, T, "1",
                  CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                  CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t x_id2 = cubec_ast_create_identifier(checker, T, "x");
  node_t x_id3 = cubec_ast_create_identifier(checker, T, "x");
  node_t x_plus_1 = cubec_ast_create_binary(checker, T, "+", x_id2, one);
  node_t incr = cubec_ast_create_assignment(checker, T, "=", x_id3, x_plus_1);

  node_t continue_stmt = cubec_ast_create_continue_stmt(checker, T);
  vec_t body_stmts = cubec_ast_create_vec(checker, true);
  vec_push(body_stmts, continue_stmt);
  node_t body = cubec_ast_create_block(checker, T, body_stmts);

  node_t for_stmt = cubec_ast_create_for_stmt(checker, T, init, cond, incr, body);

  comptime_signal_t sig = comptime_eval_exec_stmt(checker->comptime_eval, checker, for_stmt);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);
  allocator_free(allocator, &for_stmt);
  context_dispose(checker);
}

/* ===== string slice ===== */

TEST_F(dt_comptime_eval, eval_string_slice) {
  context_t checker = context_create(allocator);
  node_t host = cubec_ast_create_string(checker, T, "hello world");
  node_t start = cubec_ast_create_numeric(checker, T, "0",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t length = cubec_ast_create_numeric(checker, T, "5",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t slice = cubec_ast_create_slice_expr(checker, T, host, start, length);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, slice);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_STRING);
  EXPECT_STREQ(comptime_value_get_string(v), "hello");
  allocator_free(allocator, &slice);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, eval_string_slice_middle) {
  context_t checker = context_create(allocator);
  node_t host = cubec_ast_create_string(checker, T, "hello world");
  node_t start = cubec_ast_create_numeric(checker, T, "6",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t length = cubec_ast_create_numeric(checker, T, "5",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t slice = cubec_ast_create_slice_expr(checker, T, host, start, length);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, slice);
  ASSERT_NE(v, nullptr);
  EXPECT_STREQ(comptime_value_get_string(v), "world");
  allocator_free(allocator, &slice);
  context_dispose(checker);
}

/* ===== composite slice ===== */

TEST_F(dt_comptime_eval, eval_composite_slice) {
  context_t checker = context_create(allocator);
  /* create an array composite with 5 int values [10,20,30,40,50] */
  size_t elem_size = checker->builtin_i32->impl->size;  /* 4 bytes per i32 */
  size_t data_size = 5 * elem_size;
  comptime_value_t comp = comptime_value_create_composite(
      allocator, NULL, checker->builtin_i32, data_size);
  for (int i = 0; i < 5; i++) {
    comptime_value_t elem = comptime_value_create_int(allocator,
                            (i + 1) * 10, (i + 1) * 10, 32, true, checker->builtin_i32);
    comptime_value_set_index(comp, i, elem, allocator);
    allocator_free(allocator, &elem);
  }

  /* bind composite to env so slice can find it via identifier */
  comptime_env_bind_value(checker->comptime_eval->global_env, checker->comptime_eval->valloc, "arr", comp);

  node_t host = cubec_ast_create_identifier(checker, T, "arr");
  node_t start = cubec_ast_create_numeric(checker, T, "1",
                    CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                    CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t length = cubec_ast_create_numeric(checker, T, "3",
                    CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                    CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t slice = cubec_ast_create_slice_expr(checker, T, host, start, length);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, slice);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_COMPOSITE);
  EXPECT_EQ(v->composite.data_size, 3u * elem_size);
  /* elements should be 20, 30, 40 */
  comptime_value_t e0 = comptime_value_get_index(v, 0, allocator);
  comptime_value_t e1 = comptime_value_get_index(v, 1, allocator);
  comptime_value_t e2 = comptime_value_get_index(v, 2, allocator);
  ASSERT_NE(e0, nullptr);
  ASSERT_NE(e1, nullptr);
  ASSERT_NE(e2, nullptr);
  EXPECT_EQ(e0->int_val.s, 20);
  EXPECT_EQ(e1->int_val.s, 30);
  EXPECT_EQ(e2->int_val.s, 40);
  allocator_free(allocator, &e0);
  allocator_free(allocator, &e1);
  allocator_free(allocator, &e2);
  allocator_free(allocator, &slice);
  context_dispose(checker);
}

/* ===== do-while ===== */

TEST_F(dt_comptime_eval, do_while_basic) {
  context_t checker = context_create(allocator);

  /* do { x = x + 1 } while(x < 3) with x initialized to 0 */
  comptime_env_bind_value(checker->comptime_eval->global_env, checker->comptime_eval->valloc, "x",
      comptime_value_create_int(allocator, 0, 0, 32, true, checker->builtin_i32));

  node_t x_id = cubec_ast_create_identifier(checker, T, "x");
  node_t one = cubec_ast_create_numeric(checker, T, "1",
                  CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                  CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t x_id_b = cubec_ast_create_identifier(checker, T, "x");
  node_t x_plus_1 = cubec_ast_create_binary(checker, T, "+", x_id, one);
  node_t asgn = cubec_ast_create_assignment(checker, T, "=", x_id_b, x_plus_1);
  node_t expr_stmt = cubec_ast_create_expr_stmt(checker, T, asgn);
  vec_t body_stmts = cubec_ast_create_vec(checker, true);
  vec_push(body_stmts, expr_stmt);
  node_t body = cubec_ast_create_block(checker, T, body_stmts);

  node_t x_id2 = cubec_ast_create_identifier(checker, T, "x");
  node_t three = cubec_ast_create_numeric(checker, T, "3",
                  CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                  CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t cond = cubec_ast_create_binary(checker, T, "<", x_id2, three);

  node_t dw = cubec_ast_create_do_while_stmt(checker, T, body, cond);
  comptime_signal_t sig = comptime_eval_exec_stmt(checker->comptime_eval, checker, dw);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);

  comptime_value_t v = comptime_env_lookup_value(checker->comptime_eval->global_env, checker->comptime_eval->valloc, "x");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 3);
  allocator_free(allocator, &dw);
  context_dispose(checker);
}

/* ===== composite field assignment ===== */

TEST_F(dt_comptime_eval, composite_field_assign) {
  context_t checker = context_create(allocator);

  /* create a struct type {x: i32, y: i32} via the type system */
  semantic_type_t pt_type = semantic_type_create_named(allocator, "Point", TYPE_STRUCT);
  struct symbol *fx = (struct symbol *)allocator_alloc(allocator, sizeof(struct symbol));
  fx->name = "x"; fx->kind = SYMBOL_FIELD; fx->field.type = checker->builtin_i32;
  fx->field.index = 0; fx->field.offset = 0; fx->field.is_pub = false;
  struct symbol *fy = (struct symbol *)allocator_alloc(allocator, sizeof(struct symbol));
  fy->name = "y"; fy->kind = SYMBOL_FIELD; fy->field.type = checker->builtin_i32;
  fy->field.index = 1; fy->field.offset = 4; fy->field.is_pub = false;
  vec_init_t fvi = {.auto_dispose = false};
  vec_t type_fields = (vec_t)allocator_create(allocator, &g_vec_type, &fvi);
  vec_push(type_fields, fx);
  vec_push(type_fields, fy);
  pt_type->impl->struct_type.fields = type_fields;
  type_layout_compute(pt_type, 8);

  /* create composite {x: 10, y: 20} */
  comptime_value_t comp = comptime_value_create_composite(
      allocator, pt_type, NULL, pt_type->impl->size);
  comptime_value_t init_x = comptime_value_create_int(allocator, 10, 10, 32, true, checker->builtin_i32);
  comptime_value_t init_y = comptime_value_create_int(allocator, 20, 20, 32, true, checker->builtin_i32);
  comptime_value_set_field(comp, "x", init_x, allocator);
  comptime_value_set_field(comp, "y", init_y, allocator);
  allocator_free(allocator, &init_x);
  allocator_free(allocator, &init_y);
  comptime_env_bind_value(checker->comptime_eval->global_env, checker->comptime_eval->valloc, "pt", comp);

  /* pt.x = 99 */
  node_t pt_id = cubec_ast_create_identifier(checker, T, "pt");
  node_t target = cubec_ast_create_member(checker, T, pt_id, "x");
  node_t val = cubec_ast_create_numeric(checker, T, "99",
                  CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                  CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t asgn = cubec_ast_create_assignment(checker, T, "=", target, val);
  comptime_value_t v = comptime_eval_expr(checker->comptime_eval, checker, asgn);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 99);

  /* verify the composite was updated */
  comptime_value_t updated = comptime_env_lookup_value(checker->comptime_eval->global_env, checker->comptime_eval->valloc, "pt");
  ASSERT_NE(updated, nullptr);
  comptime_value_t x_field = comptime_value_get_field(updated, "x", allocator);
  ASSERT_NE(x_field, nullptr);
  EXPECT_EQ(x_field->int_val.s, 99);
  allocator_free(allocator, &x_field);
  allocator_free(allocator, &asgn);
  /* Dispose checker first — comptime values reference pt_type via comp->type,
     so pt_type must remain valid until comptime_allocator_dispose runs. */
  context_dispose(checker);
  allocator_free(allocator, &pt_type);
  allocator_free(allocator, &fx);
  allocator_free(allocator, &fy);
}

/* ===== test block execution ===== */

TEST_F(dt_comptime_eval, test_block_empty) {
  /* test "basic" { } */
  vec_t body_stmts = cubec_ast_create_vec(ctx, true);
  node_t body = cubec_ast_create_block(ctx, T, body_stmts);
  node_t test = cubec_ast_create_test_stmt(ctx, T, "basic", body);

  vec_t stmts = cubec_ast_create_vec(ctx, true);
  vec_push(stmts, test);
  node_t prog = cubec_ast_create_program(ctx, T, stmts);

  context_t checker = context_create(allocator);
  context_check_program(checker, prog);
  EXPECT_EQ(checker->error_count, 0u);

  allocator_free(allocator, &prog);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, test_block_assert_true) {
  /* test "ok" { assert(true); } */
  vec_t call_args = cubec_ast_create_vec(ctx, true);
  vec_push(call_args, cubec_ast_create_identifier(ctx, T, "true"));
  node_t assert_call = cubec_ast_create_call(ctx, T,
      cubec_ast_create_identifier(ctx, T, "assert"), call_args);
  node_t expr_stmt = cubec_ast_create_expr_stmt(ctx, T, assert_call);

  vec_t body_stmts = cubec_ast_create_vec(ctx, true);
  vec_push(body_stmts, expr_stmt);
  node_t body = cubec_ast_create_block(ctx, T, body_stmts);
  node_t test = cubec_ast_create_test_stmt(ctx, T, "ok", body);

  vec_t stmts = cubec_ast_create_vec(ctx, true);
  vec_push(stmts, create_builtin_assert(ctx));
  vec_push(stmts, test);
  node_t prog = cubec_ast_create_program(ctx, T, stmts);

  context_t checker = context_create(allocator);
  context_check_program(checker, prog);
  EXPECT_EQ(checker->error_count, 0u);

  allocator_free(allocator, &prog);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, test_block_assert_false) {
  /* test "fail" { assert(false); } */
  vec_t call_args = cubec_ast_create_vec(ctx, true);
  vec_push(call_args, cubec_ast_create_identifier(ctx, T, "false"));
  node_t assert_call = cubec_ast_create_call(ctx, T,
      cubec_ast_create_identifier(ctx, T, "assert"), call_args);
  node_t expr_stmt = cubec_ast_create_expr_stmt(ctx, T, assert_call);

  vec_t body_stmts = cubec_ast_create_vec(ctx, true);
  vec_push(body_stmts, expr_stmt);
  node_t body = cubec_ast_create_block(ctx, T, body_stmts);
  node_t test = cubec_ast_create_test_stmt(ctx, T, "fail", body);

  vec_t stmts = cubec_ast_create_vec(ctx, true);
  vec_push(stmts, create_builtin_assert(ctx));
  vec_push(stmts, test);
  node_t prog = cubec_ast_create_program(ctx, T, stmts);

  context_t checker = context_create(allocator);
  context_check_program(checker, prog);
  EXPECT_GT(checker->error_count, 0u);

  allocator_free(allocator, &prog);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, test_block_assert_eq) {
  /* test "math" { assert(1 + 1 == 2); } */
  node_t one = cubec_ast_create_numeric(ctx, T, "1",
                        CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                        CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t one2 = cubec_ast_create_numeric(ctx, T, "1",
                         CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                         CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t two = cubec_ast_create_numeric(ctx, T, "2",
                        CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                        CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t add = cubec_ast_create_binary(ctx, T, "+", one, one2);
  node_t eq = cubec_ast_create_binary(ctx, T, "==", add, two);

  vec_t call_args = cubec_ast_create_vec(ctx, true);
  vec_push(call_args, eq);
  node_t assert_call = cubec_ast_create_call(ctx, T,
      cubec_ast_create_identifier(ctx, T, "assert"), call_args);
  node_t expr_stmt = cubec_ast_create_expr_stmt(ctx, T, assert_call);

  vec_t body_stmts = cubec_ast_create_vec(ctx, true);
  vec_push(body_stmts, expr_stmt);
  node_t body = cubec_ast_create_block(ctx, T, body_stmts);
  node_t test = cubec_ast_create_test_stmt(ctx, T, "math", body);

  vec_t stmts = cubec_ast_create_vec(ctx, true);
  vec_push(stmts, create_builtin_assert(ctx));
  vec_push(stmts, test);
  node_t prog = cubec_ast_create_program(ctx, T, stmts);

  context_t checker = context_create(allocator);
  context_check_program(checker, prog);
  EXPECT_EQ(checker->error_count, 0u);

  allocator_free(allocator, &prog);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, test_block_assert_with_message) {
  /* test "msg" { assert(false, "custom message"); } */
  vec_t call_args = cubec_ast_create_vec(ctx, true);
  vec_push(call_args, cubec_ast_create_identifier(ctx, T, "false"));
  vec_push(call_args, cubec_ast_create_string(ctx, T, "custom message"));
  node_t assert_call = cubec_ast_create_call(ctx, T,
      cubec_ast_create_identifier(ctx, T, "assert"), call_args);
  node_t expr_stmt = cubec_ast_create_expr_stmt(ctx, T, assert_call);

  vec_t body_stmts = cubec_ast_create_vec(ctx, true);
  vec_push(body_stmts, expr_stmt);
  node_t body = cubec_ast_create_block(ctx, T, body_stmts);
  node_t test = cubec_ast_create_test_stmt(ctx, T, "msg", body);

  vec_t stmts = cubec_ast_create_vec(ctx, true);
  vec_push(stmts, create_builtin_assert(ctx));
  vec_push(stmts, test);
  node_t prog = cubec_ast_create_program(ctx, T, stmts);

  context_t checker = context_create(allocator);
  context_check_program(checker, prog);
  EXPECT_GT(checker->error_count, 0u);

  allocator_free(allocator, &prog);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, test_block_uses_global_var) {
  /* comptime var x: i32 = 42; test "var" { assert(x == 42); } */
  node_t type_id = cubec_ast_create_identifier(ctx, T, "i32");
  node_t init_val = cubec_ast_create_numeric(ctx, T, "42",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t var = cubec_ast_create_var_decl_stmt(ctx, T, "x",
                    type_id, init_val, false, false, false, true, false);

  node_t x_id = cubec_ast_create_identifier(ctx, T, "x");
  node_t forty_two = cubec_ast_create_numeric(ctx, T, "42",
                        CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                        CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t eq = cubec_ast_create_binary(ctx, T, "==", x_id, forty_two);

  vec_t call_args = cubec_ast_create_vec(ctx, true);
  vec_push(call_args, eq);
  node_t assert_call = cubec_ast_create_call(ctx, T,
      cubec_ast_create_identifier(ctx, T, "assert"), call_args);
  node_t expr_stmt = cubec_ast_create_expr_stmt(ctx, T, assert_call);

  vec_t body_stmts = cubec_ast_create_vec(ctx, true);
  vec_push(body_stmts, expr_stmt);
  node_t body = cubec_ast_create_block(ctx, T, body_stmts);
  node_t test = cubec_ast_create_test_stmt(ctx, T, "var", body);

  vec_t stmts = cubec_ast_create_vec(ctx, true);
  vec_push(stmts, create_builtin_assert(ctx));
  vec_push(stmts, var);
  vec_push(stmts, test);
  node_t prog = cubec_ast_create_program(ctx, T, stmts);

  context_t checker = context_create(allocator);
  context_check_program(checker, prog);
  EXPECT_EQ(checker->error_count, 0u);

  allocator_free(allocator, &prog);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, test_block_calls_function) {
  /* func double(n: i32): i32 { return n + n; } test "fn" { assert(double(21) == 42); } */
  node_t n_type = cubec_ast_create_identifier(ctx, T, "i32");
  vec_t fn_args = cubec_ast_create_vec(ctx, true);
  vec_push(fn_args, cubec_ast_create_func_arg(ctx, T, "n", n_type));

  node_t n_id = cubec_ast_create_identifier(ctx, T, "n");
  node_t n_id2 = cubec_ast_create_identifier(ctx, T, "n");
  node_t n_plus_n = cubec_ast_create_binary(ctx, T, "+", n_id, n_id2);
  node_t ret_stmt = cubec_ast_create_return_stmt(ctx, T, n_plus_n);
  vec_t fn_body_stmts = cubec_ast_create_vec(ctx, true);
  vec_push(fn_body_stmts, ret_stmt);
  node_t fn_body = cubec_ast_create_block(ctx, T, fn_body_stmts);

  node_t ret_type = cubec_ast_create_identifier(ctx, T, "i32");
  node_t fn = cubec_ast_create_func_stmt(ctx, T, "double", fn_args,
                ret_type, fn_body, false, false, false, false, false, false);

  /* assert(double(21) == 42) */
  node_t arg21 = cubec_ast_create_numeric(ctx, T, "21",
                        CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                        CUBEC_LITERAL_NUMERIC_TYPE_I32);
  vec_t call_args = cubec_ast_create_vec(ctx, true);
  vec_push(call_args, arg21);
  node_t callee = cubec_ast_create_identifier(ctx, T, "double");
  node_t call_double = cubec_ast_create_call(ctx, T, callee, call_args);

  node_t forty_two = cubec_ast_create_numeric(ctx, T, "42",
                        CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                        CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t eq = cubec_ast_create_binary(ctx, T, "==", call_double, forty_two);

  vec_t assert_args = cubec_ast_create_vec(ctx, true);
  vec_push(assert_args, eq);
  node_t assert_call = cubec_ast_create_call(ctx, T,
      cubec_ast_create_identifier(ctx, T, "assert"), assert_args);
  node_t expr_stmt = cubec_ast_create_expr_stmt(ctx, T, assert_call);

  vec_t body_stmts = cubec_ast_create_vec(ctx, true);
  vec_push(body_stmts, expr_stmt);
  node_t body = cubec_ast_create_block(ctx, T, body_stmts);
  node_t test = cubec_ast_create_test_stmt(ctx, T, "fn", body);

  vec_t stmts = cubec_ast_create_vec(ctx, true);
  vec_push(stmts, create_builtin_assert(ctx));
  vec_push(stmts, fn);
  vec_push(stmts, test);
  node_t prog = cubec_ast_create_program(ctx, T, stmts);

  context_t checker = context_create(allocator);
  context_check_program(checker, prog);
  EXPECT_EQ(checker->error_count, 0u);

  allocator_free(allocator, &prog);
  context_dispose(checker);
}

TEST_F(dt_comptime_eval, test_block_failure_isolation) {
  /* Two tests: first fails, second passes — error_count should be 1 (not abort) */
  /* test "fail" { assert(false); } test "pass" { assert(true); } */

  /* First test: assert(false) */
  vec_t args1 = cubec_ast_create_vec(ctx, true);
  vec_push(args1, cubec_ast_create_identifier(ctx, T, "false"));
  node_t call1 = cubec_ast_create_call(ctx, T,
      cubec_ast_create_identifier(ctx, T, "assert"), args1);
  node_t es1 = cubec_ast_create_expr_stmt(ctx, T, call1);
  vec_t body1_stmts = cubec_ast_create_vec(ctx, true);
  vec_push(body1_stmts, es1);
  node_t body1 = cubec_ast_create_block(ctx, T, body1_stmts);
  node_t test1 = cubec_ast_create_test_stmt(ctx, T, "fail", body1);

  /* Second test: assert(true) */
  vec_t args2 = cubec_ast_create_vec(ctx, true);
  vec_push(args2, cubec_ast_create_identifier(ctx, T, "true"));
  node_t call2 = cubec_ast_create_call(ctx, T,
      cubec_ast_create_identifier(ctx, T, "assert"), args2);
  node_t es2 = cubec_ast_create_expr_stmt(ctx, T, call2);
  vec_t body2_stmts = cubec_ast_create_vec(ctx, true);
  vec_push(body2_stmts, es2);
  node_t body2 = cubec_ast_create_block(ctx, T, body2_stmts);
  node_t test2 = cubec_ast_create_test_stmt(ctx, T, "pass", body2);

  vec_t stmts = cubec_ast_create_vec(ctx, true);
  vec_push(stmts, create_builtin_assert(ctx));
  vec_push(stmts, test1);
  vec_push(stmts, test2);
  node_t prog = cubec_ast_create_program(ctx, T, stmts);

  context_t checker = context_create(allocator);
  context_check_program(checker, prog);
  /* First test fails (1 error), but second test still runs */
  EXPECT_EQ(checker->error_count, 1u);

  allocator_free(allocator, &prog);
  context_dispose(checker);
}

/* ===== extern function cannot be called at comptime ===== */

TEST_F(dt_comptime_eval, extern_call_in_comptime_error) {
  const char *src =
      "extern func read_file(path: *u8): []u8;\n"
      "comptime func try_read(): []u8 {\n"
      "    return read_file(\"test\");\n"
      "}\n";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", src);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t prog = read_program_node(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(prog, nullptr);

  context_t checker = context_create(allocator);
  source_cache_load(checker->sources, "test.cubec", src, false);
  context_check_program(checker, prog);

  /* Should report error: cannot call extern at comptime */
  EXPECT_GT(checker->error_count, 0u);

  context_dispose(checker);
  allocator_free(allocator, &prog);
  allocator_free(allocator, &tokens);
}