#include "cubec/literal_numeric.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_literal_numeric : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

TEST_F(dt_literal_numeric, parse_non_numeric_returns_null) {
  const char *source = "\"hello\"";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_numeric(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_numeric, parse_integer) {
  const char *source = "42";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_numeric(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_NUMERIC);

  cubec_literal_numeric_t literal = (cubec_literal_numeric_t)node;
  EXPECT_EQ(literal->kind, CUBEC_LITERAL_NUMERIC_KIND_INTEGER);
  EXPECT_EQ(literal->numeric_type, CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT);
  EXPECT_STREQ(string_get(literal->value), "42");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_numeric, parse_float) {
  const char *source = "3.14";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_numeric(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_NUMERIC);

  cubec_literal_numeric_t literal = (cubec_literal_numeric_t)node;
  EXPECT_EQ(literal->kind, CUBEC_LITERAL_NUMERIC_KIND_FLOAT);
  EXPECT_EQ(literal->numeric_type, CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT);
  EXPECT_STREQ(string_get(literal->value), "3.14");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_numeric, parse_integer_with_i32_suffix) {
  const char *source = "42i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_numeric(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_NUMERIC);

  cubec_literal_numeric_t literal = (cubec_literal_numeric_t)node;
  EXPECT_EQ(literal->kind, CUBEC_LITERAL_NUMERIC_KIND_INTEGER);
  EXPECT_EQ(literal->numeric_type, CUBEC_LITERAL_NUMERIC_TYPE_I32);
  EXPECT_STREQ(string_get(literal->value), "42i32");
  EXPECT_EQ(node->location.end.column, 5);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_numeric, parse_integer_with_u64_suffix) {
  const char *source = "100u64";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_numeric(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_NUMERIC);

  cubec_literal_numeric_t literal = (cubec_literal_numeric_t)node;
  EXPECT_EQ(literal->kind, CUBEC_LITERAL_NUMERIC_KIND_INTEGER);
  EXPECT_EQ(literal->numeric_type, CUBEC_LITERAL_NUMERIC_TYPE_U64);
  EXPECT_STREQ(string_get(literal->value), "100u64");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_numeric, parse_float_with_f32_suffix) {
  const char *source = "3.14f32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_numeric(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_NUMERIC);

  cubec_literal_numeric_t literal = (cubec_literal_numeric_t)node;
  EXPECT_EQ(literal->kind, CUBEC_LITERAL_NUMERIC_KIND_FLOAT);
  EXPECT_EQ(literal->numeric_type, CUBEC_LITERAL_NUMERIC_TYPE_F32);
  EXPECT_STREQ(string_get(literal->value), "3.14f32");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_numeric, parse_float_with_f64_suffix) {
  const char *source = "2.718281828f64";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_numeric(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_NUMERIC);

  cubec_literal_numeric_t literal = (cubec_literal_numeric_t)node;
  EXPECT_EQ(literal->kind, CUBEC_LITERAL_NUMERIC_KIND_FLOAT);
  EXPECT_EQ(literal->numeric_type, CUBEC_LITERAL_NUMERIC_TYPE_F64);
  EXPECT_STREQ(string_get(literal->value), "2.718281828f64");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_numeric, parse_hex_integer) {
  const char *source = "0xFF";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_numeric(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_NUMERIC);

  cubec_literal_numeric_t literal = (cubec_literal_numeric_t)node;
  EXPECT_EQ(literal->kind, CUBEC_LITERAL_NUMERIC_KIND_INTEGER);
  EXPECT_EQ(literal->numeric_type, CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT);
  EXPECT_STREQ(string_get(literal->value), "0xFF");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_numeric, parse_octal_integer) {
  const char *source = "0o755";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_numeric(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_NUMERIC);

  cubec_literal_numeric_t literal = (cubec_literal_numeric_t)node;
  EXPECT_EQ(literal->kind, CUBEC_LITERAL_NUMERIC_KIND_INTEGER);
  EXPECT_EQ(literal->numeric_type, CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT);
  EXPECT_STREQ(string_get(literal->value), "0o755");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_numeric, parse_binary_integer) {
  const char *source = "0b1010";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_literal_numeric(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_NUMERIC);

  cubec_literal_numeric_t literal = (cubec_literal_numeric_t)node;
  EXPECT_EQ(literal->kind, CUBEC_LITERAL_NUMERIC_KIND_INTEGER);
  EXPECT_EQ(literal->numeric_type, CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT);
  EXPECT_STREQ(string_get(literal->value), "0b1010");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_literal_numeric, type_to_string) {
  EXPECT_STREQ(cubec_literal_numeric_type_to_string(CUBEC_LITERAL_NUMERIC_TYPE_I8),
               "i8");
  EXPECT_STREQ(cubec_literal_numeric_type_to_string(CUBEC_LITERAL_NUMERIC_TYPE_I16),
               "i16");
  EXPECT_STREQ(cubec_literal_numeric_type_to_string(CUBEC_LITERAL_NUMERIC_TYPE_I32),
               "i32");
  EXPECT_STREQ(cubec_literal_numeric_type_to_string(CUBEC_LITERAL_NUMERIC_TYPE_I64),
               "i64");
  EXPECT_STREQ(cubec_literal_numeric_type_to_string(CUBEC_LITERAL_NUMERIC_TYPE_U8),
               "u8");
  EXPECT_STREQ(cubec_literal_numeric_type_to_string(CUBEC_LITERAL_NUMERIC_TYPE_U16),
               "u16");
  EXPECT_STREQ(cubec_literal_numeric_type_to_string(CUBEC_LITERAL_NUMERIC_TYPE_U32),
               "u32");
  EXPECT_STREQ(cubec_literal_numeric_type_to_string(CUBEC_LITERAL_NUMERIC_TYPE_U64),
               "u64");
  EXPECT_STREQ(cubec_literal_numeric_type_to_string(CUBEC_LITERAL_NUMERIC_TYPE_F16),
               "f16");
  EXPECT_STREQ(cubec_literal_numeric_type_to_string(CUBEC_LITERAL_NUMERIC_TYPE_F32),
               "f32");
  EXPECT_STREQ(cubec_literal_numeric_type_to_string(CUBEC_LITERAL_NUMERIC_TYPE_F64),
               "f64");
  EXPECT_STREQ(cubec_literal_numeric_type_to_string(CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT),
               "");
}