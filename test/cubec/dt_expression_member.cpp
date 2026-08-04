#include "core/string.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "cubec/expression_member.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_member : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

TEST_F(dt_expression_member, single_member_access) {
  const char *source = "obj.field";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t member = (cubec_expression_member_t)node;
  ASSERT_NE(member->host, nullptr);
  EXPECT_EQ(member->host->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)member->host)->value), "obj");

  ASSERT_NE(member->field, nullptr);
  EXPECT_STREQ(string_get(member->field->value), "field");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_member, chained_member_access) {
  const char *source = "a.b.c";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  /* Outer: (a.b).c */
  cubec_expression_member_t outer = (cubec_expression_member_t)node;
  ASSERT_NE(outer->field, nullptr);
  EXPECT_STREQ(string_get(outer->field->value), "c");

  /* Inner host: a.b */
  ASSERT_NE(outer->host, nullptr);
  EXPECT_EQ(outer->host->kind, CUBEC_NODE_EXPRESSION_MEMBER);
  cubec_expression_member_t inner = (cubec_expression_member_t)outer->host;
  ASSERT_NE(inner->field, nullptr);
  EXPECT_STREQ(string_get(inner->field->value), "b");

  /* Innermost host: a */
  ASSERT_NE(inner->host, nullptr);
  EXPECT_EQ(inner->host->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)inner->host)->value), "a");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_member, triple_chain) {
  const char *source = "x.y.z.w";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  /* Walk the chain: x.y.z.w → field=w, host=x.y.z */
  cubec_expression_member_t m1 = (cubec_expression_member_t)node;
  EXPECT_STREQ(string_get(m1->field->value), "w");
  EXPECT_EQ(m1->host->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t m2 = (cubec_expression_member_t)m1->host;
  EXPECT_STREQ(string_get(m2->field->value), "z");
  EXPECT_EQ(m2->host->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t m3 = (cubec_expression_member_t)m2->host;
  EXPECT_STREQ(string_get(m3->field->value), "y");
  EXPECT_EQ(m3->host->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)m3->host)->value), "x");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_member, no_dot_returns_atom) {
  const char *source = "hello";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* No dot → returns the identifier atom directly */
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_member, member_with_spaces) {
  const char *source = "obj . field";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t member = (cubec_expression_member_t)node;
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)member->host)->value), "obj");
  EXPECT_STREQ(string_get(member->field->value), "field");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_member, consume_all_tokens) {
  const char *source = "a.b";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  /* position should be past all tokens (a, ., b → 3) */
  EXPECT_EQ(position, 3);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_member, numeric_after_dot_returns_null) {
  const char *source = "a.123";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* Only "a" was parsed; ".123" is not a valid member access */
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_member, member_on_string) {
  /* NOTE: member_on_numeric ("42.field") is blocked by a tokenizer bug:
   * read_numeric_token tries to parse "42." as a float and THROWs
   * on the non-numeric suffix. Use a string literal as host instead. */
  const char *source = "\"hello\".len";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t member = (cubec_expression_member_t)node;
  EXPECT_EQ(member->host->kind, CUBEC_NODE_LITERAL_STRING);
  EXPECT_STREQ(string_get(member->field->value), "len");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_member, write_member) {
  const char *source = "a.b";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  writer_t writer = (writer_t)allocator_create(allocator, &g_writer_type, NULL);
  write_expression(writer, node);
  string_t result = writer_get_string(writer); const char *output = string_get(result);
  EXPECT_STREQ(output, "a.b");

  allocator_free(allocator, &result); allocator_free(allocator, &writer);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
