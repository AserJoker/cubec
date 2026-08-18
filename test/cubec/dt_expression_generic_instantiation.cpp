#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/expression.h"
#include "cubec/expression_subscript.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_call.h"
#include "cubec/expression_member.h"
#include "cubec/expression_spread.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_expression_generic_instantiation : public CubecTest {
protected:
};

/* --------------------------------------------------------------------------
 *  Basic generic instantiation: zero, one, and multiple arguments
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_generic_instantiation, no_args) {
  const char *source = "foo[]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);

  cubec_expression_subscript_t gi =
      (cubec_expression_subscript_t)node;
  ASSERT_NE(gi->host, nullptr);
  EXPECT_EQ(gi->host->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)gi->host)->value), "foo");
  EXPECT_EQ(vec_get_size(gi->arguments), 0);

  /* foo, [, ] */
  EXPECT_EQ(position, 3);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_generic_instantiation, one_arg) {
  const char *source = "foo[a]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);

  cubec_expression_subscript_t gi =
      (cubec_expression_subscript_t)node;
  EXPECT_EQ(vec_get_size(gi->arguments), 1);

  node_t arg0 = (node_t)vec_get(gi->arguments, 0);
  ASSERT_NE(arg0, nullptr);
  EXPECT_EQ(arg0->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)arg0)->value), "a");

  /* foo, [, a, ] */
  EXPECT_EQ(position, 4);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_generic_instantiation, two_args) {
  const char *source = "foo[a, b]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);

  cubec_expression_subscript_t gi =
      (cubec_expression_subscript_t)node;
  EXPECT_EQ(vec_get_size(gi->arguments), 2);

  node_t arg0 = (node_t)vec_get(gi->arguments, 0);
  EXPECT_EQ(arg0->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)arg0)->value), "a");

  node_t arg1 = (node_t)vec_get(gi->arguments, 1);
  EXPECT_EQ(arg1->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)arg1)->value), "b");

  /* foo, [, a, ,, WS, b, ] */
  EXPECT_EQ(position, 7);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Generic instantiation with numeric and compound arguments
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_generic_instantiation, numeric_args) {
  const char *source = "foo[0, 42]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);

  cubec_expression_subscript_t gi =
      (cubec_expression_subscript_t)node;
  EXPECT_EQ(vec_get_size(gi->arguments), 2);

  node_t arg0 = (node_t)vec_get(gi->arguments, 0);
  EXPECT_EQ(arg0->kind, CUBEC_NODE_LITERAL_NUMERIC);

  node_t arg1 = (node_t)vec_get(gi->arguments, 1);
  EXPECT_EQ(arg1->kind, CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_generic_instantiation, binary_arg) {
  const char *source = "foo[a + b, c]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);

  cubec_expression_subscript_t gi =
      (cubec_expression_subscript_t)node;
  EXPECT_EQ(vec_get_size(gi->arguments), 2);

  /* First arg: binary expression a + b */
  node_t arg0 = (node_t)vec_get(gi->arguments, 0);
  EXPECT_EQ(arg0->kind, CUBEC_NODE_EXPRESSION_BINARY);
  cubec_expression_binary_t bin = (cubec_expression_binary_t)arg0;
  EXPECT_STREQ(string_get(bin->opt), "+");

  /* Second arg: identifier c */
  node_t arg1 = (node_t)vec_get(gi->arguments, 1);
  EXPECT_EQ(arg1->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Spread arguments
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_generic_instantiation, spread_arg) {
  const char *source = "foo[...a]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);

  cubec_expression_subscript_t gi =
      (cubec_expression_subscript_t)node;
  EXPECT_EQ(vec_get_size(gi->arguments), 1);

  node_t arg0 = (node_t)vec_get(gi->arguments, 0);
  ASSERT_NE(arg0, nullptr);
  EXPECT_EQ(arg0->kind, CUBEC_NODE_EXPRESSION_SPREAD);

  cubec_expression_spread_t spread = (cubec_expression_spread_t)arg0;
  ASSERT_NE(spread->value, nullptr);
  EXPECT_EQ(spread->value->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_generic_instantiation, mixed_spread) {
  const char *source = "foo[a, ...b, c]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);

  cubec_expression_subscript_t gi =
      (cubec_expression_subscript_t)node;
  EXPECT_EQ(vec_get_size(gi->arguments), 3);

  /* arg0: regular identifier */
  EXPECT_EQ(((node_t)vec_get(gi->arguments, 0))->kind,
            CUBEC_NODE_LITERAL_IDENTIFIER);

  /* arg1: spread */
  EXPECT_EQ(((node_t)vec_get(gi->arguments, 1))->kind,
            CUBEC_NODE_EXPRESSION_SPREAD);

  /* arg2: regular identifier */
  EXPECT_EQ(((node_t)vec_get(gi->arguments, 2))->kind,
            CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Wildcard arguments
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_generic_instantiation, wildcard_arg) {
  /* foo[?]  鈫? generic instantiation with wildcard argument */
  const char *source = "foo[?]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);

  cubec_expression_subscript_t gi =
      (cubec_expression_subscript_t)node;
  EXPECT_EQ(vec_get_size(gi->arguments), 1);

  /* Wildcard '?' is parsed as a wildcard expression node */
  node_t arg0 = (node_t)vec_get(gi->arguments, 0);
  ASSERT_NE(arg0, nullptr);
  EXPECT_EQ(arg0->kind, CUBEC_NODE_EXPRESSION_WILDCARD);

  /* foo, [, ?, ] */
  EXPECT_EQ(position, 4);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_generic_instantiation, wildcard_mixed) {
  /* foo[a, ?, b]  鈫? generic instantiation with wildcard among other args */
  const char *source = "foo[a, ?, b]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);

  cubec_expression_subscript_t gi =
      (cubec_expression_subscript_t)node;
  EXPECT_EQ(vec_get_size(gi->arguments), 3);

  /* arg0: identifier a */
  node_t arg0 = (node_t)vec_get(gi->arguments, 0);
  EXPECT_EQ(arg0->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(string_get(((cubec_literal_identifier_t)arg0)->value), "a");

  /* arg1: wildcard '?' */
  node_t arg1 = (node_t)vec_get(gi->arguments, 1);
  EXPECT_EQ(arg1->kind, CUBEC_NODE_EXPRESSION_WILDCARD);

  /* arg2: identifier b */
  node_t arg2 = (node_t)vec_get(gi->arguments, 2);
  EXPECT_EQ(arg2->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(string_get(((cubec_literal_identifier_t)arg2)->value), "b");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_generic_instantiation, multiple_wildcards) {
  /* foo[?, ?, ?]  鈫? generic instantiation with multiple wildcards */
  const char *source = "foo[?, ?, ?]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);

  cubec_expression_subscript_t gi =
      (cubec_expression_subscript_t)node;
  EXPECT_EQ(vec_get_size(gi->arguments), 3);

  for (size_t i = 0; i < 3; i++) {
    node_t arg = (node_t)vec_get(gi->arguments, i);
    EXPECT_EQ(arg->kind, CUBEC_NODE_EXPRESSION_WILDCARD);
  }

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Chaining: generic instantiation with call and member
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_generic_instantiation, generic_then_call) {
  /* foo[a]()  鈫? generic instantiation, then call with no args */
  const char *source = "foo[a]()";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_CALL);

  cubec_expression_call_t call = (cubec_expression_call_t)node;
  EXPECT_EQ(call->callee->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);
  EXPECT_EQ(vec_get_size(call->arguments), 0);

  cubec_expression_subscript_t gi =
      (cubec_expression_subscript_t)call->callee;
  EXPECT_EQ(vec_get_size(gi->arguments), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_generic_instantiation, generic_then_member) {
  /* foo[a].field  鈫? generic instantiation, then member access */
  const char *source = "foo[a].field";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t member = (cubec_expression_member_t)node;
  EXPECT_EQ(member->host->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);
  EXPECT_STREQ(string_get(member->field->value), "field");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_generic_instantiation, call_then_generic) {
  /* foo()[a]  鈫? call first, then generic instantiation on result */
  const char *source = "foo()[a]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);

  cubec_expression_subscript_t gi =
      (cubec_expression_subscript_t)node;
  EXPECT_EQ(gi->host->kind, CUBEC_NODE_EXPRESSION_CALL);
  EXPECT_EQ(vec_get_size(gi->arguments), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_generic_instantiation, chained_generic) {
  /* foo[a][b]  鈫? nested generic instantiation */
  const char *source = "foo[a][b]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);

  cubec_expression_subscript_t outer =
      (cubec_expression_subscript_t)node;
  EXPECT_EQ(outer->host->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);
  EXPECT_EQ(vec_get_size(outer->arguments), 1);

  cubec_expression_subscript_t inner =
      (cubec_expression_subscript_t)outer->host;
  EXPECT_EQ(inner->host->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(vec_get_size(inner->arguments), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Complex chain: generic + call + member combos
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_generic_instantiation, generic_call_member) {
  /* foo[a]().field  鈫? generic 鈫?call 鈫?member */
  const char *source = "foo[a]().field";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t member = (cubec_expression_member_t)node;
  EXPECT_EQ(member->host->kind, CUBEC_NODE_EXPRESSION_CALL);

  cubec_expression_call_t call = (cubec_expression_call_t)member->host;
  EXPECT_EQ(call->callee->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Error / negative cases
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_generic_instantiation, not_a_generic_no_bracket) {
  const char *source = "foo";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* Should just be an identifier, not a generic instantiation */
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_generic_instantiation, unclosed_bracket) {
  const char *source = "foo[a";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  /* parse error expected: unclosed bracket 鈫?recorded in diagnostics */
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  EXPECT_GT(context_get_error_count(ctx), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_generic_instantiation, trailing_comma) {
  const char *source = "foo[a, ]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  /* parse error expected: trailing comma 鈫?recorded in diagnostics */
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  EXPECT_GT(context_get_error_count(ctx), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_generic_instantiation, write_generic_instantiation) {
  const char *source = "Vec[i32]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_expression(ectx, node);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "Vec[i32]");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
