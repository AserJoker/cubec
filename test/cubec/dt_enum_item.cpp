#include "common/test_common.h"
#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/enum_item.h"
#include "cubec/expression.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_enum_item : public CubecTest {
protected:
};

TEST_F(dt_enum_item, parse_name_only) {
  const char *source = "Red";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_enum_item(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_ENUM_ITEM);

  cubec_enum_item_t item = (cubec_enum_item_t)node;
  ASSERT_NE(item->name, nullptr);
  cubec_literal_identifier_t name = (cubec_literal_identifier_t)item->name;
  EXPECT_STREQ(string_get(name->value), "Red");
  EXPECT_EQ(item->type, nullptr);
  EXPECT_EQ(item->value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_enum_item, parse_name_with_type) {
  const char *source = "Ok: u8";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_enum_item(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_ENUM_ITEM);

  cubec_enum_item_t item = (cubec_enum_item_t)node;
  ASSERT_NE(item->name, nullptr);
  cubec_literal_identifier_t name2 = (cubec_literal_identifier_t)item->name;
  EXPECT_STREQ(string_get(name2->value), "Ok");
  ASSERT_NE(item->type, nullptr);
  EXPECT_EQ(item->value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_enum_item, parse_name_with_value) {
  const char *source = "Green = 1";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_enum_item(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_ENUM_ITEM);

  cubec_enum_item_t item = (cubec_enum_item_t)node;
  ASSERT_NE(item->name, nullptr);
  cubec_literal_identifier_t name3 = (cubec_literal_identifier_t)item->name;
  EXPECT_STREQ(string_get(name3->value), "Green");
  EXPECT_EQ(item->type, nullptr);
  ASSERT_NE(item->value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_enum_item, parse_name_with_type_and_value) {
  const char *source = "Red: u8 = 0";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_enum_item(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_ENUM_ITEM);

  cubec_enum_item_t item = (cubec_enum_item_t)node;
  ASSERT_NE(item->name, nullptr);
  cubec_literal_identifier_t name = (cubec_literal_identifier_t)item->name;
  EXPECT_STREQ(string_get(name->value), "Red");
  ASSERT_NE(item->type, nullptr);
  ASSERT_NE(item->value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_enum_item, write_name_only) {
  const char *source = "Red";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_enum_item(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_enum_item(ectx, node);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "Red");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_enum_item, write_name_with_type) {
  const char *source = "Ok: u8";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_enum_item(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_enum_item(ectx, node);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "Ok: u8");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_enum_item, write_name_with_value) {
  const char *source = "Green = 1";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_enum_item(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_enum_item(ectx, node);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "Green = 1");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_enum_item, write_name_with_type_and_value) {
  const char *source = "Red: u8 = 0";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_enum_item(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_enum_item(ectx, node);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "Red: u8 = 0");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
