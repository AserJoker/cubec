#include "cubec/literal_char.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_literal_char : public Test {
protected:
  TEST_ALLOCATOR;
};

TEST_F(dt_literal_char, parse_non_char_returns_null) {
  const char *source = "123";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_char(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, tokens);
}

TEST_F(dt_literal_char, parse_simple_char) {
  const char *source = "'a'";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_char(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_CHAR);

  cubec_literal_char_t literal = (cubec_literal_char_t)node;
  EXPECT_EQ(literal->value, 'a');

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_literal_char, parse_escape_char) {
  const char *source = "'\\n'";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_char(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_CHAR);

  cubec_literal_char_t literal = (cubec_literal_char_t)node;
  EXPECT_EQ(literal->value, '\\');

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_literal_char, parse_digit_char) {
  const char *source = "'5'";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_char(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_CHAR);

  cubec_literal_char_t literal = (cubec_literal_char_t)node;
  EXPECT_EQ(literal->value, '5');

  allocator_free(allocator, node);
  allocator_free(allocator, tokens);
}

TEST_F(dt_literal_char, parse_string_returns_null) {
  const char *source = "\"hello\"";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_char(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, tokens);
}