#include "core/string.h"
#include "core/writer.h"
#include "cubec/statement_enum.h"
#include "cubec/enum_item.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/program.h"
#include "cubec/statement.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_statement_enum : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ==========================================================================
 *  Basic parsing
 * ========================================================================== */

/* ---- Basic enum: enum Color { Red, Green, Blue } ---- */

TEST_F(dt_statement_enum, basic_enum) {
  const char *source = "enum Color { Red, Green, Blue }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_enum(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_ENUM);

  cubec_statement_enum_t enum_node = (cubec_statement_enum_t)node;
  EXPECT_FALSE(enum_node->is_export);
  ASSERT_NE(enum_node->name, nullptr);
  EXPECT_EQ(enum_node->name->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(enum_node->items, nullptr);
  EXPECT_EQ(vec_get_size(enum_node->items), 3);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Empty enum ---- */

TEST_F(dt_statement_enum, enum_empty) {
  const char *source = "enum Empty { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_enum(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_ENUM);

  cubec_statement_enum_t enum_node = (cubec_statement_enum_t)node;
  ASSERT_NE(enum_node->items, nullptr);
  EXPECT_EQ(vec_get_size(enum_node->items), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Enum with type annotations ---- */

TEST_F(dt_statement_enum, enum_with_type) {
  const char *source = "enum Status { Ok: u8, Error: u8 }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_enum(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_enum_t enum_node = (cubec_statement_enum_t)node;
  ASSERT_NE(enum_node->items, nullptr);
  EXPECT_EQ(vec_get_size(enum_node->items), 2);

  /* First item has type */
  cubec_enum_item_t item0 = (cubec_enum_item_t)vec_get(enum_node->items, 0);
  ASSERT_NE(item0->type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Enum with type and value ---- */

TEST_F(dt_statement_enum, enum_with_type_and_value) {
  const char *source = "enum Color { Red: u8 = 0, Green: u8 = 1, Blue: u8 = 2 }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_enum(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_enum_t enum_node = (cubec_statement_enum_t)node;
  ASSERT_NE(enum_node->items, nullptr);
  EXPECT_EQ(vec_get_size(enum_node->items), 3);

  /* First item has type and value */
  cubec_enum_item_t item0 = (cubec_enum_item_t)vec_get(enum_node->items, 0);
  ASSERT_NE(item0->type, nullptr);
  ASSERT_NE(item0->value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Enum with value only ---- */

TEST_F(dt_statement_enum, enum_value_only) {
  const char *source = "enum Color { Red = 0, Green = 1 }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_enum(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_enum_t enum_node = (cubec_statement_enum_t)node;
  ASSERT_NE(enum_node->items, nullptr);
  EXPECT_EQ(vec_get_size(enum_node->items), 2);

  /* First item has value but no type */
  cubec_enum_item_t item0 = (cubec_enum_item_t)vec_get(enum_node->items, 0);
  EXPECT_EQ(item0->type, nullptr);
  ASSERT_NE(item0->value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Export modifier
 * ========================================================================== */

TEST_F(dt_statement_enum, export_enum) {
  const char *source = "export enum Color { Red, Green, Blue }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_enum(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_enum_t enum_node = (cubec_statement_enum_t)node;
  EXPECT_TRUE(enum_node->is_export);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_enum, non_export_enum) {
  const char *source = "enum Color { Red }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_enum(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_enum_t enum_node = (cubec_statement_enum_t)node;
  EXPECT_FALSE(enum_node->is_export);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Trailing comma
 * ========================================================================== */

TEST_F(dt_statement_enum, enum_trailing_comma) {
  const char *source = "enum Color { Red, Green, Blue, }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_enum(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_enum_t enum_node = (cubec_statement_enum_t)node;
  ASSERT_NE(enum_node->items, nullptr);
  EXPECT_EQ(vec_get_size(enum_node->items), 3);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Lifecycle: clone / move
 * ========================================================================== */

TEST_F(dt_statement_enum, clone) {
  const char *source = "enum Color { Red: u8 = 0, Green: u8 = 1 }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_enum(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_ENUM);

  cubec_statement_enum_t enum_node = (cubec_statement_enum_t)cloned;
  ASSERT_NE(enum_node->name, nullptr);
  ASSERT_NE(enum_node->items, nullptr);
  EXPECT_EQ(vec_get_size(enum_node->items), 2);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_enum, move) {
  const char *source = "enum Color { Red, Green }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_enum(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_ENUM);

  cubec_statement_enum_t enum_node = (cubec_statement_enum_t)moved;
  ASSERT_NE(enum_node->name, nullptr);
  ASSERT_NE(enum_node->items, nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Token consumption
 * ========================================================================== */

TEST_F(dt_statement_enum, consume_all_tokens) {
  const char *source = "enum Color { Red, Green, Blue }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_enum(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(position, vec_get_size(tokens) - 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Integration
 * ========================================================================== */

TEST_F(dt_statement_enum, via_read_statement) {
  const char *source = "enum Color { Red }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_ENUM);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_enum, via_read_program) {
  const char *source = "enum Color { Red }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_program_node(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_PROGRAM);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_enum, write_simple_enum) {
  const char *source = "enum Color { Red }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  writer_t writer = (writer_t)allocator_create(allocator, &g_writer_type, NULL);
  write_statement(writer, node);
  string_t result = writer_get_string(writer); const char *output = string_get(result);
  EXPECT_STREQ(output, "enum Color {\n  Red,\n}\n");
  allocator_free(allocator, &result); allocator_free(allocator, &writer);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
