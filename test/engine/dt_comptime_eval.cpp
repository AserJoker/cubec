#include "engine/comptime_eval.h"
#include "engine/comptime_value.h"
#include "engine/checker.h"
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

class dt_comptime_eval : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* ===== literal evaluation ===== */

TEST_F(dt_comptime_eval, eval_int_literal) {
  checker_t ctx = checker_create(allocator);
  node_t num = cubec_ast_create_numeric(allocator, T, "42",
                                          CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                          CUBEC_LITERAL_NUMERIC_TYPE_I32);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, num);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(v->int_val.s, 42);
  allocator_free(allocator, &num);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_bool_literal) {
  checker_t ctx = checker_create(allocator);
  /* true = identifier "true" */
  node_t b = cubec_ast_create_identifier(allocator, T, "true");
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, b);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_BOOL);
  EXPECT_TRUE(v->bool_val);
  allocator_free(allocator, &b);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_string_literal) {
  checker_t ctx = checker_create(allocator);
  node_t s = cubec_ast_create_string(allocator, T, "hello");
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, s);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_STRING);
  EXPECT_STREQ(comptime_value_get_string(v), "hello");
  allocator_free(allocator, &s);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_char_literal) {
  checker_t ctx = checker_create(allocator);
  node_t c = cubec_ast_create_char(allocator, T, 'X');
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, c);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_CHAR);
  EXPECT_EQ(v->char_val, 'X');
  allocator_free(allocator, &c);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_nil_literal) {
  checker_t ctx = checker_create(allocator);
  node_t n = cubec_ast_create_identifier(allocator, T, "nil");
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, n);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_NIL);
  allocator_free(allocator, &n);
  checker_dispose(ctx);
}

/* ===== arithmetic ===== */

TEST_F(dt_comptime_eval, eval_add) {
  checker_t ctx = checker_create(allocator);
  node_t left = cubec_ast_create_numeric(allocator, T, "10",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(allocator, T, "20",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(allocator, T, "+", left, right);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(v->int_val.s, 30);
  allocator_free(allocator, &bin);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_sub) {
  checker_t ctx = checker_create(allocator);
  node_t left = cubec_ast_create_numeric(allocator, T, "50",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(allocator, T, "20",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(allocator, T, "-", left, right);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 30);
  allocator_free(allocator, &bin);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_mul) {
  checker_t ctx = checker_create(allocator);
  node_t left = cubec_ast_create_numeric(allocator, T, "6",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(allocator, T, "7",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(allocator, T, "*", left, right);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 42);
  allocator_free(allocator, &bin);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_float_add) {
  checker_t ctx = checker_create(allocator);
  node_t left = cubec_ast_create_numeric(allocator, T, "1.5",
                                           CUBEC_LITERAL_NUMERIC_KIND_FLOAT,
                                           CUBEC_LITERAL_NUMERIC_TYPE_F64);
  node_t right = cubec_ast_create_numeric(allocator, T, "2.5",
                                            CUBEC_LITERAL_NUMERIC_KIND_FLOAT,
                                            CUBEC_LITERAL_NUMERIC_TYPE_F64);
  node_t bin = cubec_ast_create_binary(allocator, T, "+", left, right);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_FLOAT);
  EXPECT_DOUBLE_EQ(v->float_val.value, 4.0);
  allocator_free(allocator, &bin);
  checker_dispose(ctx);
}

/* ===== comparison ===== */

TEST_F(dt_comptime_eval, eval_eq) {
  checker_t ctx = checker_create(allocator);
  node_t left = cubec_ast_create_numeric(allocator, T, "5",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(allocator, T, "5",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(allocator, T, "==", left, right);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_BOOL);
  EXPECT_TRUE(v->bool_val);
  allocator_free(allocator, &bin);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_neq) {
  checker_t ctx = checker_create(allocator);
  node_t left = cubec_ast_create_numeric(allocator, T, "5",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(allocator, T, "3",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(allocator, T, "!=", left, right);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(v->bool_val);
  allocator_free(allocator, &bin);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_lt) {
  checker_t ctx = checker_create(allocator);
  node_t left = cubec_ast_create_numeric(allocator, T, "3",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(allocator, T, "5",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(allocator, T, "<", left, right);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(v->bool_val);
  allocator_free(allocator, &bin);
  checker_dispose(ctx);
}

/* ===== unary ===== */

TEST_F(dt_comptime_eval, eval_negate) {
  checker_t ctx = checker_create(allocator);
  node_t right = cubec_ast_create_numeric(allocator, T, "7",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(allocator, T, "-", NULL, right);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(v->int_val.s, -7);
  allocator_free(allocator, &bin);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_logical_not) {
  checker_t ctx = checker_create(allocator);
  node_t right = cubec_ast_create_identifier(allocator, T, "true");
  node_t bin = cubec_ast_create_binary(allocator, T, "!", NULL, right);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_BOOL);
  EXPECT_FALSE(v->bool_val);
  allocator_free(allocator, &bin);
  checker_dispose(ctx);
}

/* ===== logical binary ===== */

TEST_F(dt_comptime_eval, eval_logical_and) {
  checker_t ctx = checker_create(allocator);
  node_t left = cubec_ast_create_identifier(allocator, T, "true");
  node_t right = cubec_ast_create_identifier(allocator, T, "false");
  node_t bin = cubec_ast_create_binary(allocator, T, "&&", left, right);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_BOOL);
  EXPECT_FALSE(v->bool_val);
  allocator_free(allocator, &bin);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_logical_or) {
  checker_t ctx = checker_create(allocator);
  node_t left = cubec_ast_create_identifier(allocator, T, "false");
  node_t right = cubec_ast_create_identifier(allocator, T, "true");
  node_t bin = cubec_ast_create_binary(allocator, T, "||", left, right);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_TRUE(v->bool_val);
  allocator_free(allocator, &bin);
  checker_dispose(ctx);
}

/* ===== ternary ===== */

TEST_F(dt_comptime_eval, eval_ternary_true) {
  checker_t ctx = checker_create(allocator);
  node_t cond = cubec_ast_create_identifier(allocator, T, "true");
  node_t then_b = cubec_ast_create_numeric(allocator, T, "10",
                                              CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                              CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t else_b = cubec_ast_create_numeric(allocator, T, "20",
                                              CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                              CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t tern = cubec_ast_create_ternary(allocator, T, cond, then_b, else_b);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, tern);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 10);
  allocator_free(allocator, &tern);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_ternary_false) {
  checker_t ctx = checker_create(allocator);
  node_t cond = cubec_ast_create_identifier(allocator, T, "false");
  node_t then_b = cubec_ast_create_numeric(allocator, T, "10",
                                              CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                              CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t else_b = cubec_ast_create_numeric(allocator, T, "20",
                                              CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                              CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t tern = cubec_ast_create_ternary(allocator, T, cond, then_b, else_b);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, tern);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 20);
  allocator_free(allocator, &tern);
  checker_dispose(ctx);
}

/* ===== typeof / sizeof / alignof ===== */

TEST_F(dt_comptime_eval, eval_typeof) {
  checker_t ctx = checker_create(allocator);
  node_t inner = cubec_ast_create_identifier(allocator, T, "i32");
  node_t tof = cubec_ast_create_typeof(allocator, T, inner);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, tof);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_TYPE);
  EXPECT_EQ(v->type_val, ctx->builtin_i32);
  allocator_free(allocator, &tof);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_sizeof) {
  checker_t ctx = checker_create(allocator);
  node_t inner = cubec_ast_create_identifier(allocator, T, "i32");
  node_t sof = cubec_ast_create_sizeof(allocator, T, inner);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, sof);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(v->int_val.s, 4);
  allocator_free(allocator, &sof);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_alignof) {
  checker_t ctx = checker_create(allocator);
  node_t inner = cubec_ast_create_identifier(allocator, T, "i32");
  node_t aof = cubec_ast_create_alignof(allocator, T, inner);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, aof);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT);
  EXPECT_GE(v->int_val.s, 1);
  allocator_free(allocator, &aof);
  checker_dispose(ctx);
}

/* ===== variable declaration & access ===== */

TEST_F(dt_comptime_eval, comptime_var_decl_and_use) {
  checker_t ctx = checker_create(allocator);
  /* comptime x: i32 = 42; */
  node_t type_id = cubec_ast_create_identifier(allocator, T, "i32");
  node_t init = cubec_ast_create_numeric(allocator, T, "42",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t var = cubec_ast_create_var_decl_stmt(allocator, T, "x",
                    type_id, init, false, false, false, true);
  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, var);
  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_check_program(ctx, prog);

  /* The comptime var should have been evaluated and bound in the env */
  comptime_value_t v = comptime_env_lookup(ctx->comptime_eval->global_env, "x");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(v->int_val.s, 42);

  allocator_free(allocator, &prog);
  checker_dispose(ctx);
}

/* ===== if statement ===== */

TEST_F(dt_comptime_eval, comptime_if_true) {
  checker_t ctx = checker_create(allocator);
  /* comptime if true { ... } */
  vec_t empty_vec = cubec_ast_create_vec(allocator, true);
  node_t then_body = cubec_ast_create_block(allocator, T, empty_vec);
  node_t cond = cubec_ast_create_identifier(allocator, T, "true");
  node_t cif = cubec_ast_create_if_stmt(allocator, T, cond, then_body, NULL);
  comptime_signal_t sig = comptime_eval_exec_comptime_if(ctx->comptime_eval, ctx, cif);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);
  allocator_free(allocator, &cif);
  checker_dispose(ctx);
}

/* ===== for statement ===== */

TEST_F(dt_comptime_eval, comptime_for_loop) {
  checker_t ctx = checker_create(allocator);

  /* Build: comptime for init=var sum:i32=0, cond=sum<3, incr=sum=sum+1, body=empty */
  node_t type_id = cubec_ast_create_identifier(allocator, T, "i32");
  node_t init_val = cubec_ast_create_numeric(allocator, T, "0",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t init = cubec_ast_create_var_decl_stmt(allocator, T, "sum",
                  type_id, init_val, false, false, false, true);

  node_t sum_id = cubec_ast_create_identifier(allocator, T, "sum");
  node_t three = cubec_ast_create_numeric(allocator, T, "3",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t cond = cubec_ast_create_binary(allocator, T, "<", sum_id, three);

  node_t one = cubec_ast_create_numeric(allocator, T, "1",
                  CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                  CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t sum_id2 = cubec_ast_create_identifier(allocator, T, "sum");
  node_t sum_id3 = cubec_ast_create_identifier(allocator, T, "sum");
  node_t sum_plus_1 = cubec_ast_create_binary(allocator, T, "+", sum_id2, one);
  node_t incr = cubec_ast_create_assignment(allocator, T, "=", sum_id3, sum_plus_1);

  vec_t empty_vec = cubec_ast_create_vec(allocator, true);
  node_t body = cubec_ast_create_block(allocator, T, empty_vec);

  node_t for_stmt = cubec_ast_create_for_stmt(allocator, T, init, cond, incr, body);

  comptime_signal_t sig = comptime_eval_exec_comptime_for(ctx->comptime_eval, ctx, for_stmt);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);

  allocator_free(allocator, &for_stmt);
  checker_dispose(ctx);
}

/* ===== function call ===== */

TEST_F(dt_comptime_eval, function_call) {
  checker_t ctx = checker_create(allocator);

  /* Build: func double(x: i32): i32 { return x + x; } */
  node_t x_type = cubec_ast_create_identifier(allocator, T, "i32");
  vec_t args = cubec_ast_create_vec(allocator, true);
  vec_push(args, cubec_ast_create_func_arg(allocator, T, "x", x_type));

  node_t x_id = cubec_ast_create_identifier(allocator, T, "x");
  node_t x_id2 = cubec_ast_create_identifier(allocator, T, "x");
  node_t x_plus_x = cubec_ast_create_binary(allocator, T, "+", x_id, x_id2);
  node_t ret_stmt = cubec_ast_create_return_stmt(allocator, T, x_plus_x);
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, ret_stmt);
  node_t body = cubec_ast_create_block(allocator, T, body_stmts);

  node_t ret_type = cubec_ast_create_identifier(allocator, T, "i32");
  node_t fn = cubec_ast_create_func_stmt(allocator, T, "double", args,
                ret_type, body, false, false, false, false, false, false);

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, fn);
  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_check_program(ctx, prog);

  /* Verify function is in comptime env */
  comptime_value_t fn_val = comptime_env_lookup(ctx->comptime_eval->global_env, "double");
  ASSERT_NE(fn_val, nullptr) << "function 'double' not in comptime env";
  EXPECT_EQ(fn_val->kind, COMPTIME_VALUE_FUNCTION) << "got kind=" << fn_val->kind;

  /* Now call double(21) via the evaluator */
  node_t arg21 = cubec_ast_create_numeric(allocator, T, "21",
                        CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                        CUBEC_LITERAL_NUMERIC_TYPE_I32);
  vec_t call_args = cubec_ast_create_vec(allocator, true);
  vec_push(call_args, arg21);
  node_t callee = cubec_ast_create_identifier(allocator, T, "double");
  node_t call = cubec_ast_create_call(allocator, T, callee, call_args);

  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, call);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT) << "got kind=" << v->kind;
  EXPECT_EQ(v->int_val.s, 42);

  allocator_free(allocator, &call);
  allocator_free(allocator, &prog);
  checker_dispose(ctx);
}

/* ===== comptime block ===== */

TEST_F(dt_comptime_eval, comptime_block_executes) {
  checker_t ctx = checker_create(allocator);

  /* comptime { } — empty comptime block */
  vec_t empty_vec = cubec_ast_create_vec(allocator, true);
  node_t block = cubec_ast_create_block(allocator, T, empty_vec);
  comptime_signal_t sig = comptime_eval_exec_block(ctx->comptime_eval, ctx, block);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);
  allocator_free(allocator, &block);
  checker_dispose(ctx);
}

/* ===== group expression ===== */

TEST_F(dt_comptime_eval, eval_group) {
  checker_t ctx = checker_create(allocator);
  node_t inner = cubec_ast_create_numeric(allocator, T, "42",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t grp = cubec_ast_create_group(allocator, T, inner);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, grp);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 42);
  allocator_free(allocator, &grp);
  checker_dispose(ctx);
}

/* ===== comma expression ===== */

TEST_F(dt_comptime_eval, eval_comma) {
  checker_t ctx = checker_create(allocator);
  node_t left = cubec_ast_create_numeric(allocator, T, "1",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(allocator, T, "2",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t comma = cubec_ast_create_comma(allocator, T, left, right);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, comma);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 2); /* comma returns right */
  allocator_free(allocator, &comma);
  checker_dispose(ctx);
}

/* ===== bitwise ===== */

TEST_F(dt_comptime_eval, eval_bitwise_and) {
  checker_t ctx = checker_create(allocator);
  node_t left = cubec_ast_create_numeric(allocator, T, "12",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(allocator, T, "10",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(allocator, T, "&", left, right);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT) << "got kind=" << v->kind;
  if (v->kind == COMPTIME_VALUE_INT)
    EXPECT_EQ(v->int_val.s, 8); /* 12 & 10 = 8 */
  allocator_free(allocator, &bin);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_bitwise_or) {
  checker_t ctx = checker_create(allocator);
  node_t left = cubec_ast_create_numeric(allocator, T, "12",
                                           CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                           CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t right = cubec_ast_create_numeric(allocator, T, "10",
                                            CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                                            CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t bin = cubec_ast_create_binary(allocator, T, "|", left, right);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, bin);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 14); /* 12 | 10 = 14 */
  allocator_free(allocator, &bin);
  checker_dispose(ctx);
}

/* ===== break / continue in loops ===== */

TEST_F(dt_comptime_eval, break_in_for) {
  checker_t ctx = checker_create(allocator);

  /* for init: x:i32=0, cond: true, incr: none, body: { break } */
  node_t type_id = cubec_ast_create_identifier(allocator, T, "i32");
  node_t init_val = cubec_ast_create_numeric(allocator, T, "0",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t init = cubec_ast_create_var_decl_stmt(allocator, T, "x",
                  type_id, init_val, false, false, false, false);

  node_t break_stmt = cubec_ast_create_break_stmt(allocator, T);
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, break_stmt);
  node_t body = cubec_ast_create_block(allocator, T, body_stmts);

  node_t true_id = cubec_ast_create_identifier(allocator, T, "true");
  node_t for_stmt = cubec_ast_create_for_stmt(allocator, T, init,
                    true_id, NULL, body);

  comptime_signal_t sig = comptime_eval_exec_comptime_for(ctx->comptime_eval, ctx, for_stmt);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);
  allocator_free(allocator, &for_stmt);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, continue_in_for) {
  checker_t ctx = checker_create(allocator);

  /* for init: x:i32=0, cond: x<3, incr: x=x+1, body: { continue } */
  node_t type_id = cubec_ast_create_identifier(allocator, T, "i32");
  node_t init_val = cubec_ast_create_numeric(allocator, T, "0",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t init = cubec_ast_create_var_decl_stmt(allocator, T, "x",
                  type_id, init_val, false, false, false, false);

  node_t x_id = cubec_ast_create_identifier(allocator, T, "x");
  node_t three = cubec_ast_create_numeric(allocator, T, "3",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t cond = cubec_ast_create_binary(allocator, T, "<", x_id, three);

  node_t one = cubec_ast_create_numeric(allocator, T, "1",
                  CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                  CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t x_id2 = cubec_ast_create_identifier(allocator, T, "x");
  node_t x_id3 = cubec_ast_create_identifier(allocator, T, "x");
  node_t x_plus_1 = cubec_ast_create_binary(allocator, T, "+", x_id2, one);
  node_t incr = cubec_ast_create_assignment(allocator, T, "=", x_id3, x_plus_1);

  node_t continue_stmt = cubec_ast_create_continue_stmt(allocator, T);
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, continue_stmt);
  node_t body = cubec_ast_create_block(allocator, T, body_stmts);

  node_t for_stmt = cubec_ast_create_for_stmt(allocator, T, init, cond, incr, body);

  comptime_signal_t sig = comptime_eval_exec_comptime_for(ctx->comptime_eval, ctx, for_stmt);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);
  allocator_free(allocator, &for_stmt);
  checker_dispose(ctx);
}

/* ===== string slice ===== */

TEST_F(dt_comptime_eval, eval_string_slice) {
  checker_t ctx = checker_create(allocator);
  node_t host = cubec_ast_create_string(allocator, T, "hello world");
  node_t start = cubec_ast_create_numeric(allocator, T, "0",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t length = cubec_ast_create_numeric(allocator, T, "5",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t slice = cubec_ast_create_slice_expr(allocator, T, host, start, length);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, slice);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_STRING);
  EXPECT_STREQ(comptime_value_get_string(v), "hello");
  allocator_free(allocator, &slice);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_string_slice_middle) {
  checker_t ctx = checker_create(allocator);
  node_t host = cubec_ast_create_string(allocator, T, "hello world");
  node_t start = cubec_ast_create_numeric(allocator, T, "6",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t length = cubec_ast_create_numeric(allocator, T, "5",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t slice = cubec_ast_create_slice_expr(allocator, T, host, start, length);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, slice);
  ASSERT_NE(v, nullptr);
  EXPECT_STREQ(comptime_value_get_string(v), "world");
  allocator_free(allocator, &slice);
  checker_dispose(ctx);
}

/* ===== composite slice ===== */

TEST_F(dt_comptime_eval, eval_composite_slice) {
  checker_t ctx = checker_create(allocator);
  /* create an array composite with 5 int values [10,20,30,40,50] */
  size_t elem_size = ctx->builtin_i32->impl->size;  /* 4 bytes per i32 */
  size_t data_size = 5 * elem_size;
  comptime_value_t comp = comptime_value_create_composite(
      allocator, NULL, ctx->builtin_i32, data_size);
  for (int i = 0; i < 5; i++) {
    comptime_value_t elem = comptime_value_create_int(allocator,
                            (i + 1) * 10, (i + 1) * 10, 32, true, ctx->builtin_i32);
    comptime_value_set_index(comp, i, elem);
    allocator_free(allocator, &elem);
  }

  /* bind composite to env so slice can find it via identifier */
  comptime_env_bind(ctx->comptime_eval->global_env, "arr", comp);

  node_t host = cubec_ast_create_identifier(allocator, T, "arr");
  node_t start = cubec_ast_create_numeric(allocator, T, "1",
                    CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                    CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t length = cubec_ast_create_numeric(allocator, T, "3",
                    CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                    CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t slice = cubec_ast_create_slice_expr(allocator, T, host, start, length);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, slice);
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
  checker_dispose(ctx);
}

/* ===== do-while ===== */

TEST_F(dt_comptime_eval, do_while_basic) {
  checker_t ctx = checker_create(allocator);

  /* do { x = x + 1 } while(x < 3) with x initialized to 0 */
  comptime_env_bind(ctx->comptime_eval->global_env, "x",
      comptime_value_create_int(allocator, 0, 0, 32, true, ctx->builtin_i32));

  node_t x_id = cubec_ast_create_identifier(allocator, T, "x");
  node_t one = cubec_ast_create_numeric(allocator, T, "1",
                  CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                  CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t x_id_b = cubec_ast_create_identifier(allocator, T, "x");
  node_t x_plus_1 = cubec_ast_create_binary(allocator, T, "+", x_id, one);
  node_t asgn = cubec_ast_create_assignment(allocator, T, "=", x_id_b, x_plus_1);
  node_t expr_stmt = cubec_ast_create_expr_stmt(allocator, T, asgn);
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, expr_stmt);
  node_t body = cubec_ast_create_block(allocator, T, body_stmts);

  node_t x_id2 = cubec_ast_create_identifier(allocator, T, "x");
  node_t three = cubec_ast_create_numeric(allocator, T, "3",
                  CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                  CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t cond = cubec_ast_create_binary(allocator, T, "<", x_id2, three);

  node_t dw = cubec_ast_create_do_while_stmt(allocator, T, body, cond);
  comptime_signal_t sig = comptime_eval_exec_stmt(ctx->comptime_eval, ctx, dw);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);

  comptime_value_t v = comptime_env_lookup(ctx->comptime_eval->global_env, "x");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 3);
  allocator_free(allocator, &dw);
  checker_dispose(ctx);
}

/* ===== composite field assignment ===== */

TEST_F(dt_comptime_eval, composite_field_assign) {
  checker_t ctx = checker_create(allocator);

  /* create a struct type {x: i32, y: i32} via the type system */
  semantic_type_t pt_type = semantic_type_create_named(allocator, "Point", TYPE_STRUCT);
  struct symbol *fx = (struct symbol *)allocator_alloc(allocator, sizeof(struct symbol));
  fx->name = "x"; fx->kind = SYMBOL_FIELD; fx->field.type = ctx->builtin_i32;
  fx->field.index = 0; fx->field.offset = 0; fx->field.is_pub = false;
  struct symbol *fy = (struct symbol *)allocator_alloc(allocator, sizeof(struct symbol));
  fy->name = "y"; fy->kind = SYMBOL_FIELD; fy->field.type = ctx->builtin_i32;
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
  comptime_value_t init_x = comptime_value_create_int(allocator, 10, 10, 32, true, ctx->builtin_i32);
  comptime_value_t init_y = comptime_value_create_int(allocator, 20, 20, 32, true, ctx->builtin_i32);
  comptime_value_set_field(comp, "x", init_x);
  comptime_value_set_field(comp, "y", init_y);
  allocator_free(allocator, &init_x);
  allocator_free(allocator, &init_y);
  comptime_env_bind(ctx->comptime_eval->global_env, "pt", comp);

  /* pt.x = 99 */
  node_t pt_id = cubec_ast_create_identifier(allocator, T, "pt");
  node_t target = cubec_ast_create_member(allocator, T, pt_id, "x");
  node_t val = cubec_ast_create_numeric(allocator, T, "99",
                  CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                  CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t asgn = cubec_ast_create_assignment(allocator, T, "=", target, val);
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, asgn);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 99);

  /* verify the composite was updated */
  comptime_value_t updated = comptime_env_lookup(ctx->comptime_eval->global_env, "pt");
  ASSERT_NE(updated, nullptr);
  comptime_value_t x_field = comptime_value_get_field(updated, "x", allocator);
  ASSERT_NE(x_field, nullptr);
  EXPECT_EQ(x_field->int_val.s, 99);
  allocator_free(allocator, &x_field);
  allocator_free(allocator, &asgn);
  /* pt_type is not in ctx->all_types, so we must free it manually */
  allocator_free(allocator, &pt_type);
  allocator_free(allocator, &fx);
  allocator_free(allocator, &fy);
  checker_dispose(ctx);
}

/* ===== test block execution ===== */

TEST_F(dt_comptime_eval, test_block_empty) {
  /* test "basic" { } */
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  node_t body = cubec_ast_create_block(allocator, T, body_stmts);
  node_t test = cubec_ast_create_test_stmt(allocator, T, "basic", body);

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, test);
  node_t prog = cubec_ast_create_program(allocator, T, stmts);

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);
  EXPECT_EQ(ctx->error_count, 0u);

  allocator_free(allocator, &prog);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, test_block_assert_true) {
  /* test "ok" { assert(true); } */
  vec_t call_args = cubec_ast_create_vec(allocator, true);
  vec_push(call_args, cubec_ast_create_identifier(allocator, T, "true"));
  node_t assert_call = cubec_ast_create_call(allocator, T,
      cubec_ast_create_identifier(allocator, T, "assert"), call_args);
  node_t expr_stmt = cubec_ast_create_expr_stmt(allocator, T, assert_call);

  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, expr_stmt);
  node_t body = cubec_ast_create_block(allocator, T, body_stmts);
  node_t test = cubec_ast_create_test_stmt(allocator, T, "ok", body);

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, test);
  node_t prog = cubec_ast_create_program(allocator, T, stmts);

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);
  EXPECT_EQ(ctx->error_count, 0u);

  allocator_free(allocator, &prog);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, test_block_assert_false) {
  /* test "fail" { assert(false); } */
  vec_t call_args = cubec_ast_create_vec(allocator, true);
  vec_push(call_args, cubec_ast_create_identifier(allocator, T, "false"));
  node_t assert_call = cubec_ast_create_call(allocator, T,
      cubec_ast_create_identifier(allocator, T, "assert"), call_args);
  node_t expr_stmt = cubec_ast_create_expr_stmt(allocator, T, assert_call);

  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, expr_stmt);
  node_t body = cubec_ast_create_block(allocator, T, body_stmts);
  node_t test = cubec_ast_create_test_stmt(allocator, T, "fail", body);

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, test);
  node_t prog = cubec_ast_create_program(allocator, T, stmts);

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);
  EXPECT_GT(ctx->error_count, 0u);

  allocator_free(allocator, &prog);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, test_block_assert_eq) {
  /* test "math" { assert(1 + 1 == 2); } */
  node_t one = cubec_ast_create_numeric(allocator, T, "1",
                        CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                        CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t one2 = cubec_ast_create_numeric(allocator, T, "1",
                         CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                         CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t two = cubec_ast_create_numeric(allocator, T, "2",
                        CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                        CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t add = cubec_ast_create_binary(allocator, T, "+", one, one2);
  node_t eq = cubec_ast_create_binary(allocator, T, "==", add, two);

  vec_t call_args = cubec_ast_create_vec(allocator, true);
  vec_push(call_args, eq);
  node_t assert_call = cubec_ast_create_call(allocator, T,
      cubec_ast_create_identifier(allocator, T, "assert"), call_args);
  node_t expr_stmt = cubec_ast_create_expr_stmt(allocator, T, assert_call);

  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, expr_stmt);
  node_t body = cubec_ast_create_block(allocator, T, body_stmts);
  node_t test = cubec_ast_create_test_stmt(allocator, T, "math", body);

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, test);
  node_t prog = cubec_ast_create_program(allocator, T, stmts);

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);
  EXPECT_EQ(ctx->error_count, 0u);

  allocator_free(allocator, &prog);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, test_block_assert_with_message) {
  /* test "msg" { assert(false, "custom message"); } */
  vec_t call_args = cubec_ast_create_vec(allocator, true);
  vec_push(call_args, cubec_ast_create_identifier(allocator, T, "false"));
  vec_push(call_args, cubec_ast_create_string(allocator, T, "custom message"));
  node_t assert_call = cubec_ast_create_call(allocator, T,
      cubec_ast_create_identifier(allocator, T, "assert"), call_args);
  node_t expr_stmt = cubec_ast_create_expr_stmt(allocator, T, assert_call);

  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, expr_stmt);
  node_t body = cubec_ast_create_block(allocator, T, body_stmts);
  node_t test = cubec_ast_create_test_stmt(allocator, T, "msg", body);

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, test);
  node_t prog = cubec_ast_create_program(allocator, T, stmts);

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);
  EXPECT_GT(ctx->error_count, 0u);

  allocator_free(allocator, &prog);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, test_block_uses_global_var) {
  /* comptime var x: i32 = 42; test "var" { assert(x == 42); } */
  node_t type_id = cubec_ast_create_identifier(allocator, T, "i32");
  node_t init_val = cubec_ast_create_numeric(allocator, T, "42",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t var = cubec_ast_create_var_decl_stmt(allocator, T, "x",
                    type_id, init_val, false, false, false, true);

  node_t x_id = cubec_ast_create_identifier(allocator, T, "x");
  node_t forty_two = cubec_ast_create_numeric(allocator, T, "42",
                        CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                        CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t eq = cubec_ast_create_binary(allocator, T, "==", x_id, forty_two);

  vec_t call_args = cubec_ast_create_vec(allocator, true);
  vec_push(call_args, eq);
  node_t assert_call = cubec_ast_create_call(allocator, T,
      cubec_ast_create_identifier(allocator, T, "assert"), call_args);
  node_t expr_stmt = cubec_ast_create_expr_stmt(allocator, T, assert_call);

  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, expr_stmt);
  node_t body = cubec_ast_create_block(allocator, T, body_stmts);
  node_t test = cubec_ast_create_test_stmt(allocator, T, "var", body);

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, var);
  vec_push(stmts, test);
  node_t prog = cubec_ast_create_program(allocator, T, stmts);

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);
  EXPECT_EQ(ctx->error_count, 0u);

  allocator_free(allocator, &prog);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, test_block_calls_function) {
  /* func double(n: i32): i32 { return n + n; } test "fn" { assert(double(21) == 42); } */
  node_t n_type = cubec_ast_create_identifier(allocator, T, "i32");
  vec_t fn_args = cubec_ast_create_vec(allocator, true);
  vec_push(fn_args, cubec_ast_create_func_arg(allocator, T, "n", n_type));

  node_t n_id = cubec_ast_create_identifier(allocator, T, "n");
  node_t n_id2 = cubec_ast_create_identifier(allocator, T, "n");
  node_t n_plus_n = cubec_ast_create_binary(allocator, T, "+", n_id, n_id2);
  node_t ret_stmt = cubec_ast_create_return_stmt(allocator, T, n_plus_n);
  vec_t fn_body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(fn_body_stmts, ret_stmt);
  node_t fn_body = cubec_ast_create_block(allocator, T, fn_body_stmts);

  node_t ret_type = cubec_ast_create_identifier(allocator, T, "i32");
  node_t fn = cubec_ast_create_func_stmt(allocator, T, "double", fn_args,
                ret_type, fn_body, false, false, false, false, false, false);

  /* assert(double(21) == 42) */
  node_t arg21 = cubec_ast_create_numeric(allocator, T, "21",
                        CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                        CUBEC_LITERAL_NUMERIC_TYPE_I32);
  vec_t call_args = cubec_ast_create_vec(allocator, true);
  vec_push(call_args, arg21);
  node_t callee = cubec_ast_create_identifier(allocator, T, "double");
  node_t call_double = cubec_ast_create_call(allocator, T, callee, call_args);

  node_t forty_two = cubec_ast_create_numeric(allocator, T, "42",
                        CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                        CUBEC_LITERAL_NUMERIC_TYPE_I32);
  node_t eq = cubec_ast_create_binary(allocator, T, "==", call_double, forty_two);

  vec_t assert_args = cubec_ast_create_vec(allocator, true);
  vec_push(assert_args, eq);
  node_t assert_call = cubec_ast_create_call(allocator, T,
      cubec_ast_create_identifier(allocator, T, "assert"), assert_args);
  node_t expr_stmt = cubec_ast_create_expr_stmt(allocator, T, assert_call);

  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, expr_stmt);
  node_t body = cubec_ast_create_block(allocator, T, body_stmts);
  node_t test = cubec_ast_create_test_stmt(allocator, T, "fn", body);

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, fn);
  vec_push(stmts, test);
  node_t prog = cubec_ast_create_program(allocator, T, stmts);

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);
  EXPECT_EQ(ctx->error_count, 0u);

  allocator_free(allocator, &prog);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, test_block_failure_isolation) {
  /* Two tests: first fails, second passes — error_count should be 1 (not abort) */
  /* test "fail" { assert(false); } test "pass" { assert(true); } */

  /* First test: assert(false) */
  vec_t args1 = cubec_ast_create_vec(allocator, true);
  vec_push(args1, cubec_ast_create_identifier(allocator, T, "false"));
  node_t call1 = cubec_ast_create_call(allocator, T,
      cubec_ast_create_identifier(allocator, T, "assert"), args1);
  node_t es1 = cubec_ast_create_expr_stmt(allocator, T, call1);
  vec_t body1_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body1_stmts, es1);
  node_t body1 = cubec_ast_create_block(allocator, T, body1_stmts);
  node_t test1 = cubec_ast_create_test_stmt(allocator, T, "fail", body1);

  /* Second test: assert(true) */
  vec_t args2 = cubec_ast_create_vec(allocator, true);
  vec_push(args2, cubec_ast_create_identifier(allocator, T, "true"));
  node_t call2 = cubec_ast_create_call(allocator, T,
      cubec_ast_create_identifier(allocator, T, "assert"), args2);
  node_t es2 = cubec_ast_create_expr_stmt(allocator, T, call2);
  vec_t body2_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body2_stmts, es2);
  node_t body2 = cubec_ast_create_block(allocator, T, body2_stmts);
  node_t test2 = cubec_ast_create_test_stmt(allocator, T, "pass", body2);

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, test1);
  vec_push(stmts, test2);
  node_t prog = cubec_ast_create_program(allocator, T, stmts);

  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);
  /* First test fails (1 error), but second test still runs */
  EXPECT_EQ(ctx->error_count, 1u);

  allocator_free(allocator, &prog);
  checker_dispose(ctx);
}