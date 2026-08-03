#include "common/test_common.h"
#include "core/string.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "cubec/initialize_field.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include "cubec/node.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_initialize_field : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

TEST_F(dt_expression_initialize_field, basic_initialize_field) {
  const char *source = ".name = 42";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_initialize_field(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_INITIALIZE_LIST_FIELD);

  cubec_initialize_field_t field = (cubec_initialize_field_t)node;
  ASSERT_NE(field->field, nullptr);
  EXPECT_STREQ(string_get(field->field->value), "name");

  ASSERT_NE(field->value, nullptr);
  EXPECT_EQ(field->value->kind, CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_initialize_field, string_value) {
  const char *source = ".field = \"hello\"";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_initialize_field(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_INITIALIZE_LIST_FIELD);

  cubec_initialize_field_t field = (cubec_initialize_field_t)node;
  EXPECT_STREQ(string_get(field->field->value), "field");
  ASSERT_NE(field->value, nullptr);
  EXPECT_EQ(field->value->kind, CUBEC_NODE_LITERAL_STRING);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_initialize_field, identifier_value) {
  const char *source = ".x = y";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_initialize_field(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_INITIALIZE_LIST_FIELD);

  cubec_initialize_field_t field = (cubec_initialize_field_t)node;
  EXPECT_STREQ(string_get(field->field->value), "x");
  ASSERT_NE(field->value, nullptr);
  EXPECT_EQ(field->value->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_initialize_field, with_spaces) {
  const char *source = ".field   =   123";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_initialize_field(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_INITIALIZE_LIST_FIELD);

  cubec_initialize_field_t field = (cubec_initialize_field_t)node;
  EXPECT_STREQ(string_get(field->field->value), "field");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_initialize_field, binary_expression_value) {
  const char *source = ".result = a + b";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_initialize_field(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_INITIALIZE_LIST_FIELD);

  cubec_initialize_field_t field = (cubec_initialize_field_t)node;
  EXPECT_STREQ(string_get(field->field->value), "result");
  ASSERT_NE(field->value, nullptr);
  EXPECT_EQ(field->value->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_initialize_field, no_dot_returns_null) {
  const char *source = "field = 42";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_initialize_field(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_initialize_field, no_equals_returns_null) {
  const char *source = ".field 42";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_initialize_field(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_initialize_field, missing_identifier_after_dot) {
  const char *source = ". = 42";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_initialize_field(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_initialize_field, numeric_after_dot_is_not_identifier) {
  const char *source = ".123 = 42";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_initialize_field(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_initialize_field, clone) {
  const char *source = ".name = 42";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t original = read_initialize_field(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(original, nullptr);

  node_t cloned = (node_t)value_clone(allocator, original);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_INITIALIZE_LIST_FIELD);

  cubec_initialize_field_t field = (cubec_initialize_field_t)cloned;
  EXPECT_STREQ(string_get(field->field->value), "name");
  EXPECT_EQ(field->value->kind, CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &original);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_initialize_field, move) {
  const char *source = ".name = 42";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t original = read_initialize_field(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(original, nullptr);

  node_t moved = (node_t)value_move(allocator, original);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_INITIALIZE_LIST_FIELD);
  /* value_move transfers ownership but does not NULL the source for typed
   * objects */

  allocator_free(allocator, &moved);
  allocator_free(allocator, &original);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_initialize_field, nested_expression_in_value) {
  const char *source = ".field = (a + b) * c";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_initialize_field(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_INITIALIZE_LIST_FIELD);

  cubec_initialize_field_t field = (cubec_initialize_field_t)node;
  EXPECT_STREQ(string_get(field->field->value), "field");
  ASSERT_NE(field->value, nullptr);
  EXPECT_EQ(field->value->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_initialize_field, consume_all_tokens) {
  const char *source = ".x = 1";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_initialize_field(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  /* Verify the position moved past the = and value tokens */
  EXPECT_GT(position, 3);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_initialize_field, write_field) {
  const char *source = ".Foo{.x = 1}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  writer_t writer = (writer_t)allocator_create(allocator, &g_writer_type, NULL);
  write_expression(writer, node);
  const char *output = string_get(writer_get_string(writer));
  EXPECT_STREQ(output, ".Foo{\n  .x = 1,\n}");
  allocator_free(allocator, &writer);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_initialize_field, write_single_field) {
  const char *source = ".x = 1";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_initialize_field(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  writer_t writer = (writer_t)allocator_create(allocator, &g_writer_type, NULL);
  write_initialize_field(writer, node);
  const char *output = string_get(writer_get_string(writer));
  EXPECT_STREQ(output, ".x = 1");

  allocator_free(allocator, &writer);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
