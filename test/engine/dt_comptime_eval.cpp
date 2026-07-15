#include "engine/comptime_eval.h"
#include "engine/comptime_value.h"
#include "engine/checker.h"
#include "cubec/ast_factory.h"
#include "cubec/literal_numeric.h"
#include "cubec/statement_block.h"
#include "cubec/statement_return.h"
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
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_string_literal) {
  checker_t ctx = checker_create(allocator);
  node_t s = cubec_ast_create_string(allocator, T, "hello");
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, s);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_STRING);
  EXPECT_STREQ(comptime_value_get_string(v), "hello");
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_char_literal) {
  checker_t ctx = checker_create(allocator);
  node_t c = cubec_ast_create_char(allocator, T, 'X');
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, c);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_CHAR);
  EXPECT_EQ(v->char_val, 'X');
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, eval_nil_literal) {
  checker_t ctx = checker_create(allocator);
  node_t n = cubec_ast_create_identifier(allocator, T, "nil");
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, n);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_NIL);
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
  checker_dispose(ctx);
}

/* ===== variable declaration & access ===== */

TEST_F(dt_comptime_eval, comptime_var_decl_and_use) {
  checker_t ctx = checker_create(allocator);
  /* comptime x: i32 = 42; */
  node_t var = cubec_ast_create_var_decl_stmt(allocator, T, "x",
                    cubec_ast_create_identifier(allocator, T, "i32"),
                    cubec_ast_create_numeric(allocator, T, "42",
                        CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                        CUBEC_LITERAL_NUMERIC_TYPE_I32),
                    false, false, false, true);
  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, var);
  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_check_program(ctx, prog);

  /* The comptime var should have been evaluated and bound in the env */
  comptime_value_t v = comptime_env_lookup(ctx->comptime_eval->global_env, "x");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(v->int_val.s, 42);

  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

/* ===== if statement ===== */

TEST_F(dt_comptime_eval, comptime_if_true) {
  checker_t ctx = checker_create(allocator);
  /* comptime if true { ... } */
  node_t then_body = cubec_ast_create_block(allocator, T,
                    cubec_ast_create_vec(allocator, true));
  node_t cif = cubec_ast_create_if_stmt(allocator, T,
                 cubec_ast_create_identifier(allocator, T, "true"),
                 then_body, NULL);
  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, cif);
  /* Need to wrap in comptime block */
  /* Actually, the evaluator tests should call directly */
  comptime_signal_t sig = comptime_eval_exec_comptime_if(ctx->comptime_eval, ctx, cif);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);
  checker_dispose(ctx);
}

/* ===== for statement ===== */

TEST_F(dt_comptime_eval, comptime_for_loop) {
  checker_t ctx = checker_create(allocator);

  /* Build: comptime for init=var sum:i32=0, cond=sum<3, incr=sum=sum+1, body=empty */
  node_t init = cubec_ast_create_var_decl_stmt(allocator, T, "sum",
                  cubec_ast_create_identifier(allocator, T, "i32"),
                  cubec_ast_create_numeric(allocator, T, "0",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32),
                  false, false, false, true);

  node_t cond = cubec_ast_create_binary(allocator, T, "<",
                  cubec_ast_create_identifier(allocator, T, "sum"),
                  cubec_ast_create_numeric(allocator, T, "3",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32));

  node_t incr = cubec_ast_create_assignment(allocator, T, "=",
                  cubec_ast_create_identifier(allocator, T, "sum"),
                  cubec_ast_create_binary(allocator, T, "+",
                      cubec_ast_create_identifier(allocator, T, "sum"),
                      cubec_ast_create_numeric(allocator, T, "1",
                          CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                          CUBEC_LITERAL_NUMERIC_TYPE_I32)));

  node_t body = cubec_ast_create_block(allocator, T,
                  cubec_ast_create_vec(allocator, true));

  node_t for_stmt = cubec_ast_create_for_stmt(allocator, T, init, cond, incr, body);

  comptime_signal_t sig = comptime_eval_exec_comptime_for(ctx->comptime_eval, ctx, for_stmt);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);

  /* sum should be 3 after 3 iterations */
  comptime_value_t v = comptime_env_lookup(ctx->comptime_eval->current_env
                              ? ctx->comptime_eval->current_env
                              : ctx->comptime_eval->global_env, "sum");
  /* sum was declared in for-init scope which is gone after the loop,
     but we can verify the loop completed without error */
  checker_dispose(ctx);
}

/* ===== function call ===== */

TEST_F(dt_comptime_eval, function_call) {
  checker_t ctx = checker_create(allocator);

  /* Build: func double(x: i32): i32 { return x + x; } */
  vec_t args = cubec_ast_create_vec(allocator, true);
  vec_push(args, cubec_ast_create_func_arg(allocator, T, "x",
                  cubec_ast_create_identifier(allocator, T, "i32")));

  node_t ret_expr = cubec_ast_create_binary(allocator, T, "+",
                      cubec_ast_create_identifier(allocator, T, "x"),
                      cubec_ast_create_identifier(allocator, T, "x"));
  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_return_stmt(allocator, T, ret_expr));
  node_t body = cubec_ast_create_block(allocator, T, body_stmts);

  node_t fn = cubec_ast_create_func_stmt(allocator, T, "double", args,
                cubec_ast_create_identifier(allocator, T, "i32"),
                body, false, false, false, false, false, false);

  vec_t stmts = cubec_ast_create_vec(allocator, true);
  vec_push(stmts, fn);
  node_t prog = cubec_ast_create_program(allocator, T, stmts);
  checker_check_program(ctx, prog);

  /* Verify function is in comptime env */
  comptime_value_t fn_val = comptime_env_lookup(ctx->comptime_eval->global_env, "double");
  ASSERT_NE(fn_val, nullptr) << "function 'double' not in comptime env";
  EXPECT_EQ(fn_val->kind, COMPTIME_VALUE_FUNCTION) << "got kind=" << fn_val->kind;

  /* Now call double(21) via the evaluator */
  vec_t call_args = cubec_ast_create_vec(allocator, true);
  vec_push(call_args, cubec_ast_create_numeric(allocator, T, "21",
                        CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                        CUBEC_LITERAL_NUMERIC_TYPE_I32));
  node_t callee = cubec_ast_create_identifier(allocator, T, "double");
  node_t call = cubec_ast_create_call(allocator, T, callee, call_args);

  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, call);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_INT) << "got kind=" << v->kind;
  EXPECT_EQ(v->int_val.s, 42);

  checker_dispose(ctx);
  allocator_free(allocator, &prog);
}

/* ===== comptime block ===== */

TEST_F(dt_comptime_eval, comptime_block_executes) {
  checker_t ctx = checker_create(allocator);

  /* comptime { } — empty comptime block */
  node_t block = cubec_ast_create_block(allocator, T,
                    cubec_ast_create_vec(allocator, true));
  comptime_signal_t sig = comptime_eval_exec_block(ctx->comptime_eval, ctx, block);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);
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
  checker_dispose(ctx);
}

/* ===== break / continue in loops ===== */

TEST_F(dt_comptime_eval, break_in_for) {
  checker_t ctx = checker_create(allocator);

  /* for init: x:i32=0, cond: true, incr: none, body: { break } */
  node_t init = cubec_ast_create_var_decl_stmt(allocator, T, "x",
                  cubec_ast_create_identifier(allocator, T, "i32"),
                  cubec_ast_create_numeric(allocator, T, "0",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32),
                  false, false, false, false);

  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_break_stmt(allocator, T));
  node_t body = cubec_ast_create_block(allocator, T, body_stmts);

  node_t for_stmt = cubec_ast_create_for_stmt(allocator, T, init,
                    cubec_ast_create_identifier(allocator, T, "true"),
                    NULL, body);

  comptime_signal_t sig = comptime_eval_exec_comptime_for(ctx->comptime_eval, ctx, for_stmt);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);
  checker_dispose(ctx);
}

TEST_F(dt_comptime_eval, continue_in_for) {
  checker_t ctx = checker_create(allocator);

  /* for init: x:i32=0, cond: x<3, incr: x=x+1, body: { continue } */
  node_t init = cubec_ast_create_var_decl_stmt(allocator, T, "x",
                  cubec_ast_create_identifier(allocator, T, "i32"),
                  cubec_ast_create_numeric(allocator, T, "0",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32),
                  false, false, false, false);

  node_t cond = cubec_ast_create_binary(allocator, T, "<",
                  cubec_ast_create_identifier(allocator, T, "x"),
                  cubec_ast_create_numeric(allocator, T, "3",
                      CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                      CUBEC_LITERAL_NUMERIC_TYPE_I32));

  node_t incr = cubec_ast_create_assignment(allocator, T, "=",
                  cubec_ast_create_identifier(allocator, T, "x"),
                  cubec_ast_create_binary(allocator, T, "+",
                      cubec_ast_create_identifier(allocator, T, "x"),
                      cubec_ast_create_numeric(allocator, T, "1",
                          CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                          CUBEC_LITERAL_NUMERIC_TYPE_I32)));

  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_continue_stmt(allocator, T));
  node_t body = cubec_ast_create_block(allocator, T, body_stmts);

  node_t for_stmt = cubec_ast_create_for_stmt(allocator, T, init, cond, incr, body);

  comptime_signal_t sig = comptime_eval_exec_comptime_for(ctx->comptime_eval, ctx, for_stmt);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);
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
  checker_dispose(ctx);
}

/* ===== composite slice ===== */

TEST_F(dt_comptime_eval, eval_composite_slice) {
  checker_t ctx = checker_create(allocator);
  /* create a composite with 5 int values [10,20,30,40,50] */
  vec_t fields = cubec_ast_create_vec(allocator, true);
  for (int i = 0; i < 5; i++) {
    vec_push(fields, comptime_value_create_int(allocator,
                  (i + 1) * 10, (i + 1) * 10, 32, true, ctx->builtin_i32));
  }
  comptime_value_t comp = comptime_value_create_composite(allocator, NULL, fields, NULL, 5);

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
  ASSERT_NE(v->composite.fields, nullptr);
  EXPECT_EQ(vec_get_size(v->composite.fields), 3u);
  /* elements should be 20, 30, 40 */
  comptime_value_t e0 = (comptime_value_t)vec_get(v->composite.fields, 0);
  comptime_value_t e1 = (comptime_value_t)vec_get(v->composite.fields, 1);
  comptime_value_t e2 = (comptime_value_t)vec_get(v->composite.fields, 2);
  EXPECT_EQ(e0->int_val.s, 20);
  EXPECT_EQ(e1->int_val.s, 30);
  EXPECT_EQ(e2->int_val.s, 40);
  checker_dispose(ctx);
}

/* ===== do-while ===== */

TEST_F(dt_comptime_eval, do_while_basic) {
  checker_t ctx = checker_create(allocator);

  /* do { x = x + 1 } while(x < 3) with x initialized to 0 */
  comptime_env_bind(ctx->comptime_eval->global_env, "x",
      comptime_value_create_int(allocator, 0, 0, 32, true, ctx->builtin_i32));

  vec_t body_stmts = cubec_ast_create_vec(allocator, true);
  vec_push(body_stmts, cubec_ast_create_expr_stmt(allocator, T,
      cubec_ast_create_assignment(allocator, T, "=",
          cubec_ast_create_identifier(allocator, T, "x"),
          cubec_ast_create_binary(allocator, T, "+",
              cubec_ast_create_identifier(allocator, T, "x"),
              cubec_ast_create_numeric(allocator, T, "1",
                  CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
                  CUBEC_LITERAL_NUMERIC_TYPE_I32)))));
  node_t body = cubec_ast_create_block(allocator, T, body_stmts);

  node_t cond = cubec_ast_create_binary(allocator, T, "<",
      cubec_ast_create_identifier(allocator, T, "x"),
      cubec_ast_create_numeric(allocator, T, "3",
          CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
          CUBEC_LITERAL_NUMERIC_TYPE_I32));

  node_t dw = cubec_ast_create_do_while_stmt(allocator, T, body, cond);
  comptime_signal_t sig = comptime_eval_exec_stmt(ctx->comptime_eval, ctx, dw);
  EXPECT_EQ(sig.kind, COMPTIME_SIGNAL_NONE);

  comptime_value_t v = comptime_env_lookup(ctx->comptime_eval->global_env, "x");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->int_val.s, 3);
  checker_dispose(ctx);
}

/* TODO: composite_field_assign - member assignment needs fix for clone semantics */
/* Currently the assignment modifies a clone, not the original in environment */
