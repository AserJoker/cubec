#include "cubec/statement_export_from.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_string.h"
#include "cubec/node.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_statement_export_from : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ---- export * from "path" ---- */

TEST_F(dt_statement_export_from, export_star_from) {
  const char *source = "export * from \"std/vec\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_export_from(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_EXPORT_FROM);

  cubec_statement_export_from_t exp = (cubec_statement_export_from_t)node;
  EXPECT_TRUE(exp->is_star);
  EXPECT_EQ(exp->names, nullptr);
  EXPECT_NE(exp->path, nullptr);
  EXPECT_EQ(exp->path->kind, CUBEC_NODE_LITERAL_STRING);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- export { a, b } from "path" ---- */

TEST_F(dt_statement_export_from, export_names_from) {
  const char *source = "export { Vec, Map } from \"std/collections\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_export_from(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_EXPORT_FROM);

  cubec_statement_export_from_t exp = (cubec_statement_export_from_t)node;
  EXPECT_FALSE(exp->is_star);
  ASSERT_NE(exp->names, nullptr);
  EXPECT_EQ(vec_get_size(exp->names), 2u);
  EXPECT_NE(exp->path, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- export { single } from "path" ---- */

TEST_F(dt_statement_export_from, export_single_name) {
  const char *source = "export { Point } from \"./geom\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_export_from(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_export_from_t exp = (cubec_statement_export_from_t)node;
  EXPECT_FALSE(exp->is_star);
  ASSERT_NE(exp->names, nullptr);
  EXPECT_EQ(vec_get_size(exp->names), 1u);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- export struct does NOT match export_from ---- */

TEST_F(dt_statement_export_from, export_struct_not_matched) {
  const char *source = "export struct Foo {}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_export_from(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- export func does NOT match export_from ---- */

TEST_F(dt_statement_export_from, export_func_not_matched) {
  const char *source = "export func add(a: i32): i32 { return a; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_export_from(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Non-export keyword returns NULL ---- */

TEST_F(dt_statement_export_from, non_export_returns_null) {
  const char *source = "var x = 1;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_export_from(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Missing 'from' keyword error ---- */

TEST_F(dt_statement_export_from, missing_from_keyword) {
  const char *source = "export * \"std/vec\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_export_from(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Missing semicolon error ---- */

TEST_F(dt_statement_export_from, missing_semicolon) {
  const char *source = "export * from \"std/vec\"";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_export_from(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Missing path error ---- */

TEST_F(dt_statement_export_from, missing_path) {
  const char *source = "export * from ;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_export_from(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Consume all tokens ---- */

TEST_F(dt_statement_export_from, consume_all_tokens) {
  const char *source = "export * from \"std\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_export_from(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(position, vec_get_size(tokens) - 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Clone ---- */

TEST_F(dt_statement_export_from, clone) {
  const char *source = "export { a, b } from \"mod\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_export_from(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_EXPORT_FROM);

  cubec_statement_export_from_t exp = (cubec_statement_export_from_t)cloned;
  EXPECT_FALSE(exp->is_star);
  ASSERT_NE(exp->names, nullptr);
  EXPECT_EQ(vec_get_size(exp->names), 2u);
  EXPECT_NE(exp->path, nullptr);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Move ---- */

TEST_F(dt_statement_export_from, move) {
  const char *source = "export * from \"mod\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_export_from(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_EXPORT_FROM);

  cubec_statement_export_from_t exp = (cubec_statement_export_from_t)moved;
  EXPECT_TRUE(exp->is_star);
  EXPECT_NE(exp->path, nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Via read_program_node (top-level) ---- */

TEST_F(dt_statement_export_from, via_program_node) {
  const char *source = "export * from \"std\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_program_node(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_PROGRAM);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
