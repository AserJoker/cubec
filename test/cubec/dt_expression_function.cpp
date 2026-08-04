#include "core/string.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "cubec/expression_function.h"
#include "cubec/expression_call.h"
#include "cubec/function_capture.h"
#include "cubec/function_argument.h"
#include "cubec/generic_param.h"
#include "cubec/node.h"
#include "cubec/node_error.h"
#include "cubec/statement.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_function : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ==========================================================================
 *  Basic parsing
 * ========================================================================== */

TEST_F(dt_expression_function, no_captures_no_params) {
  const char *source = "func || () { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_FUNCTION);

  cubec_expression_function_t fn = (cubec_expression_function_t)node;
  EXPECT_EQ(fn->captures, nullptr);
  EXPECT_EQ(fn->generic_params, nullptr);
  EXPECT_NE(fn->arguments, nullptr);
  EXPECT_EQ(vec_get_size(fn->arguments), 0);
  EXPECT_EQ(fn->return_type, nullptr);
  EXPECT_NE(fn->body, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_function, simple_captures) {
  const char *source = "func |x, y| () { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_expression_function_t fn = (cubec_expression_function_t)node;
  EXPECT_NE(fn->captures, nullptr);
  EXPECT_EQ(vec_get_size(fn->captures), 2);

  cubec_function_capture_t cap0 = (cubec_function_capture_t)vec_get(fn->captures, 0);
  EXPECT_EQ(cap0->super.kind, CUBEC_NODE_FUNCTION_CAPTURE);
  EXPECT_NE(cap0->identifier, nullptr);

  cubec_function_capture_t cap1 = (cubec_function_capture_t)vec_get(fn->captures, 1);
  EXPECT_NE(cap1->identifier, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_function, empty_captures_with_params) {
  const char *source = "func || (a: i32): i32 { return a; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_expression_function_t fn = (cubec_expression_function_t)node;
  EXPECT_EQ(fn->captures, nullptr);
  EXPECT_EQ(vec_get_size(fn->arguments), 1);
  EXPECT_NE(fn->return_type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Generic
 * ========================================================================== */

TEST_F(dt_expression_function, generic_with_captures) {
  const char *source = "func |ctx| [T](x: T): T { return x; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_expression_function_t fn = (cubec_expression_function_t)node;
  EXPECT_NE(fn->captures, nullptr);
  EXPECT_NE(fn->generic_params, nullptr);
  EXPECT_EQ(vec_get_size(fn->generic_params), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_function, generic_no_captures) {
  const char *source = "func || [T](x: T): T { return x; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_expression_function_t fn = (cubec_expression_function_t)node;
  EXPECT_EQ(fn->captures, nullptr);
  EXPECT_NE(fn->generic_params, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Parameters and return type
 * ========================================================================== */

TEST_F(dt_expression_function, with_params) {
  const char *source = "func |x| (a: i32, b: i32): i32 { return a + b; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_expression_function_t fn = (cubec_expression_function_t)node;
  EXPECT_EQ(vec_get_size(fn->arguments), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_function, no_return_type) {
  const char *source = "func |x| (a: i32) { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_expression_function_t fn = (cubec_expression_function_t)node;
  EXPECT_EQ(fn->return_type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_function, complex_return_type) {
  const char *source = "func || (): *i32 { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_expression_function_t fn = (cubec_expression_function_t)node;
  EXPECT_NE(fn->return_type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_function, no_params) {
  const char *source = "func |x| (): i32 { return x; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_expression_function_t fn = (cubec_expression_function_t)node;
  EXPECT_EQ(vec_get_size(fn->arguments), 0);
  EXPECT_NE(fn->return_type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Postfix: immediate call, member access, assignment
 * ========================================================================== */

TEST_F(dt_expression_function, immediate_call) {
  const char *source = "func |x| (a: i32): i32 { return x + a; }(42)";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_CALL);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_function, chained_member) {
  const char *source = "func || (): Vec[i32] { }.field";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_function, assign_to_var) {
  const char *source = "var f = func |x| (a: i32): i32 { return a; };";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Lifecycle: clone / move
 * ========================================================================== */

TEST_F(dt_expression_function, clone) {
  const char *source = "func || (a: i32): i32 { return a; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_EXPRESSION_FUNCTION);

  cubec_expression_function_t fn = (cubec_expression_function_t)cloned;
  EXPECT_NE(fn->arguments, nullptr);
  EXPECT_NE(fn->return_type, nullptr);
  EXPECT_NE(fn->body, nullptr);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_function, clone_with_captures) {
  const char *source = "func |x, y| (): i32 { return x + y; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);

  cubec_expression_function_t fn = (cubec_expression_function_t)cloned;
  EXPECT_NE(fn->captures, nullptr);
  EXPECT_EQ(vec_get_size(fn->captures), 2);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_function, move) {
  const char *source = "func |x| (a: i32): i32 { return x + a; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_EXPRESSION_FUNCTION);

  cubec_expression_function_t fn = (cubec_expression_function_t)moved;
  EXPECT_NE(fn->captures, nullptr);
  EXPECT_NE(fn->arguments, nullptr);
  EXPECT_NE(fn->body, nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Error cases
 * ========================================================================== */

TEST_F(dt_expression_function, missing_pipe) {
  const char *source = "func x| () { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_function, missing_close_pipe) {
  const char *source = "func |x () { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_function, missing_body) {
  const char *source = "func || (): i32;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_function, not_func_returns_null) {
  const char *source = "x + y";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Integration
 * ========================================================================== */

TEST_F(dt_expression_function, via_read_expression) {
  const char *source = "func |x| (a: i32): i32 { return x + a; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_FUNCTION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_function, consume_all_tokens) {
  const char *source = "func || () { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(position, vec_get_size(tokens) - 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_function, write_closure) {
  const char *source = "func(x: i32): i32 { x }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_expression_function(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  writer_t writer = (writer_t)allocator_create(allocator, &g_writer_type, NULL);
  write_expression(writer, node);
  string_t result = writer_get_string(writer); const char *output = string_get(result);
  EXPECT_STREQ(output, "func(x: i32): i32 {\n  /* error */\n}\n");

  allocator_free(allocator, &result); allocator_free(allocator, &writer);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
