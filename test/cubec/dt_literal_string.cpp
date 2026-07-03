#include "cubec/literal_string.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_literal_string : public Test {
protected:
  TEST_ALLOCATOR;
};

TEST_F(dt_literal_string, parse_non_string_returns_null) {
  const char *source = "123";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_string(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, tokens);
}

TEST_F(dt_literal_string, parse_empty_string) {
  const char *source = "\"\"";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_string(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_STRING);

  cubec_literal_string_t literal = (cubec_literal_string_t)node;
  EXPECT_STREQ(string_get(literal->value), "");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_literal_string, parse_simple_string) {
  const char *source = "\"hello\"";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_string(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_STRING);

  cubec_literal_string_t literal = (cubec_literal_string_t)node;
  EXPECT_STREQ(string_get(literal->value), "hello");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_literal_string, parse_string_with_spaces) {
  const char *source = "\"hello world\"";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_string(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_STRING);

  cubec_literal_string_t literal = (cubec_literal_string_t)node;
  EXPECT_STREQ(string_get(literal->value), "hello world");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_literal_string, parse_string_concatenation) {
  const char *source = "\"hello\" \"world\"";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_string(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_STRING);

  cubec_literal_string_t literal = (cubec_literal_string_t)node;
  EXPECT_STREQ(string_get(literal->value), "helloworld");
  EXPECT_EQ(node->location.end.column, 15);

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_literal_string, parse_multiple_string_concatenation) {
  const char *source = "\"a\" \"b\" \"c\" \"d\"";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_string(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_STRING);

  cubec_literal_string_t literal = (cubec_literal_string_t)node;
  EXPECT_STREQ(string_get(literal->value), "abcd");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_literal_string, parse_string_with_escape_chars) {
  const char *source = "\"hello\\nworld\"";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_string(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_STRING);

  cubec_literal_string_t literal = (cubec_literal_string_t)node;
  EXPECT_STREQ(string_get(literal->value), "hello\\nworld");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_literal_string, parse_string_with_tab) {
  const char *source = "\"hello\\tworld\"";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_string(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_STRING);

  cubec_literal_string_t literal = (cubec_literal_string_t)node;
  EXPECT_STREQ(string_get(literal->value), "hello\\tworld");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_literal_string, parse_string_with_unicode) {
  const char *source = "\"你好世界\"";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_string(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_STRING);

  cubec_literal_string_t literal = (cubec_literal_string_t)node;
  EXPECT_STREQ(string_get(literal->value), "你好世界");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_literal_string, parse_string_with_whitespace_between) {
  const char *source = "\"hello\"   \"world\"";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_string(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_STRING);

  cubec_literal_string_t literal = (cubec_literal_string_t)node;
  EXPECT_STREQ(string_get(literal->value), "helloworld");

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_literal_string, parse_string_identifier_returns_null) {
  const char *source = "hello";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_string(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, tokens);
}

TEST_F(dt_literal_string, parse_string_numeric_returns_null) {
  const char *source = "123";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_string(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, tokens);
}