#include "cubec/expression.h"
#include "cubec/expression_call.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_member.h"
#include "cubec/expression_spread.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include "core/error.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_call : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* --------------------------------------------------------------------------
 *  Basic call: zero arguments
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_call, call_no_args) {
  const char *source = "foo()";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_CALL);

  cubec_expression_call_t call = (cubec_expression_call_t)node;
  ASSERT_NE(call->callee, nullptr);
  EXPECT_EQ(call->callee->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)call->callee)->value), "foo");
  EXPECT_EQ(vec_get_size(call->arguments), 0);

  /* foo, (, ) */
  EXPECT_EQ(position, 3);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

/* --------------------------------------------------------------------------
 *  Basic call: one and two arguments
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_call, call_one_arg) {
  const char *source = "foo(a)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_CALL);

  cubec_expression_call_t call = (cubec_expression_call_t)node;
  EXPECT_EQ(vec_get_size(call->arguments), 1);

  node_t arg0 = (node_t)vec_get(call->arguments, 0);
  ASSERT_NE(arg0, nullptr);
  EXPECT_EQ(arg0->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)arg0)->value), "a");

  EXPECT_EQ(position, 4); /* foo, (, a, ) */

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_call, call_two_args) {
  const char *source = "foo(a, b)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_CALL);

  cubec_expression_call_t call = (cubec_expression_call_t)node;
  EXPECT_EQ(vec_get_size(call->arguments), 2);

  node_t arg0 = (node_t)vec_get(call->arguments, 0);
  EXPECT_EQ(arg0->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)arg0)->value), "a");

  node_t arg1 = (node_t)vec_get(call->arguments, 1);
  EXPECT_EQ(arg1->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)arg1)->value), "b");

  /* foo, (, a, ,, WS, b, ) */
  EXPECT_EQ(position, 7);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

/* --------------------------------------------------------------------------
 *  Call with numeric and compound arguments
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_call, call_numeric_arg) {
  const char *source = "max(0, 42)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_CALL);

  cubec_expression_call_t call = (cubec_expression_call_t)node;
  EXPECT_EQ(vec_get_size(call->arguments), 2);

  node_t arg0 = (node_t)vec_get(call->arguments, 0);
  EXPECT_EQ(arg0->kind, CUBEC_NODE_LITERAL_NUMERIC);

  node_t arg1 = (node_t)vec_get(call->arguments, 1);
  EXPECT_EQ(arg1->kind, CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_call, call_binary_arg) {
  const char *source = "foo(a + b, c)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_CALL);

  cubec_expression_call_t call = (cubec_expression_call_t)node;
  EXPECT_EQ(vec_get_size(call->arguments), 2);

  /* First arg: binary expression a + b */
  node_t arg0 = (node_t)vec_get(call->arguments, 0);
  EXPECT_EQ(arg0->kind, CUBEC_NODE_EXPRESSION_BINARY);
  cubec_expression_binary_t bin = (cubec_expression_binary_t)arg0;
  EXPECT_STREQ(string_get(bin->opt), "+");

  /* Second arg: identifier c */
  node_t arg1 = (node_t)vec_get(call->arguments, 1);
  EXPECT_EQ(arg1->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

/* --------------------------------------------------------------------------
 *  Spread arguments
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_call, call_spread_arg) {
  const char *source = "foo(...a)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_CALL);

  cubec_expression_call_t call = (cubec_expression_call_t)node;
  EXPECT_EQ(vec_get_size(call->arguments), 1);

  node_t arg0 = (node_t)vec_get(call->arguments, 0);
  ASSERT_NE(arg0, nullptr);
  EXPECT_EQ(arg0->kind, CUBEC_NODE_EXPRESSION_SPREAD);

  cubec_expression_spread_t spread = (cubec_expression_spread_t)arg0;
  ASSERT_NE(spread->value, nullptr);
  EXPECT_EQ(spread->value->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_call, call_mixed_spread) {
  const char *source = "foo(a, ...b, c)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_CALL);

  cubec_expression_call_t call = (cubec_expression_call_t)node;
  EXPECT_EQ(vec_get_size(call->arguments), 3);

  /* arg0: regular identifier */
  EXPECT_EQ(((node_t)vec_get(call->arguments, 0))->kind,
            CUBEC_NODE_LITERAL_IDENTIFIER);

  /* arg1: spread */
  EXPECT_EQ(((node_t)vec_get(call->arguments, 1))->kind,
            CUBEC_NODE_EXPRESSION_SPREAD);

  /* arg2: regular identifier */
  EXPECT_EQ(((node_t)vec_get(call->arguments, 2))->kind,
            CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_call, call_multiple_spreads) {
  const char *source = "foo(...a, ...b)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_CALL);

  cubec_expression_call_t call = (cubec_expression_call_t)node;
  EXPECT_EQ(vec_get_size(call->arguments), 2);
  EXPECT_EQ(((node_t)vec_get(call->arguments, 0))->kind,
            CUBEC_NODE_EXPRESSION_SPREAD);
  EXPECT_EQ(((node_t)vec_get(call->arguments, 1))->kind,
            CUBEC_NODE_EXPRESSION_SPREAD);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

/* --------------------------------------------------------------------------
 *  Chaining: member access and nested calls
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_call, call_then_member) {
  /* foo().field */
  const char *source = "foo().field";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t member = (cubec_expression_member_t)node;
  EXPECT_EQ(member->host->kind, CUBEC_NODE_EXPRESSION_CALL);
  EXPECT_STREQ(string_get(member->field->value), "field");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_call, member_then_call) {
  /* foo.field() */
  const char *source = "foo.field()";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_CALL);

  cubec_expression_call_t call = (cubec_expression_call_t)node;
  EXPECT_EQ(call->callee->kind, CUBEC_NODE_EXPRESSION_MEMBER);
  EXPECT_EQ(vec_get_size(call->arguments), 0);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_call, chained_call) {
  /* foo()() */
  const char *source = "foo()()";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_CALL);

  cubec_expression_call_t outer = (cubec_expression_call_t)node;
  EXPECT_EQ(outer->callee->kind, CUBEC_NODE_EXPRESSION_CALL);
  EXPECT_EQ(vec_get_size(outer->arguments), 0);

  cubec_expression_call_t inner = (cubec_expression_call_t)outer->callee;
  EXPECT_EQ(inner->callee->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(vec_get_size(inner->arguments), 0);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

/* --------------------------------------------------------------------------
 *  Error / negative cases
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_call, not_a_call_no_paren) {
  const char *source = "foo";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* Should just be an identifier, not a call */
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_call, unclosed_paren) {
  const char *source = "foo(a";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = NULL;
  /* parse error expected: unclosed paren → THROW inside call parser */
  CATCH_ERROR(
      node = read_expression(allocator, tokens, &position, "test.cubec"),
      error_clear());
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, tokens);
}

TEST_F(dt_expression_call, trailing_comma) {
  const char *source = "foo(a, )";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = NULL;
  /* parse error expected: trailing comma → THROW inside call parser */
  CATCH_ERROR(
      node = read_expression(allocator, tokens, &position, "test.cubec"),
      error_clear());
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, tokens);
}

/* --------------------------------------------------------------------------
 *  Group expression callee (function pointer call)
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_call, group_callee) {
  /* (func_ptr)(arg)  →  call with group as callee */
  const char *source = "(fp)(a)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_CALL);

  cubec_expression_call_t call = (cubec_expression_call_t)node;
  EXPECT_EQ(call->callee->kind, CUBEC_NODE_EXPRESSION_GROUP);
  EXPECT_EQ(vec_get_size(call->arguments), 1);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}
