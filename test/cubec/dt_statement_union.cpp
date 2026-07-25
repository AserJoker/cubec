#include "cubec/statement_union.h"
#include "cubec/union_field.h"
#include "cubec/generic_param.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/program.h"
#include "cubec/statement.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_statement_union : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ==========================================================================
 *  Basic parsing
 * ========================================================================== */

TEST_F(dt_statement_union, basic_union) {
  const char *source = "union Option { value: i32; tag: u64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_union(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_UNION);

  cubec_statement_union_t union_node = (cubec_statement_union_t)node;
  EXPECT_FALSE(union_node->is_export);
  ASSERT_NE(union_node->name, nullptr);
  EXPECT_EQ(union_node->name->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(union_node->generic_params, nullptr);
  ASSERT_NE(union_node->members, nullptr);
  EXPECT_EQ(vec_get_size(union_node->members), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_union, union_empty) {
  const char *source = "union Empty { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_union(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_union_t union_node = (cubec_statement_union_t)node;
  ASSERT_NE(union_node->members, nullptr);
  EXPECT_EQ(vec_get_size(union_node->members), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_union, union_field_details) {
  const char *source = "union Data { value: i32; tag: u64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_union(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_union_t union_node = (cubec_statement_union_t)node;
  ASSERT_NE(union_node->members, nullptr);
  EXPECT_EQ(vec_get_size(union_node->members), 2);

  node_t field0 = (node_t)vec_get(union_node->members, 0);
  EXPECT_EQ(field0->kind, CUBEC_NODE_UNION_FIELD);
  cubec_union_field_t field = (cubec_union_field_t)field0;
  ASSERT_NE(field->name, nullptr);
  ASSERT_NE(field->type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Generic union
 * ========================================================================== */

TEST_F(dt_statement_union, union_generic) {
  const char *source = "union Option[T] { value: T; tag: u64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_union(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_union_t union_node = (cubec_statement_union_t)node;
  ASSERT_NE(union_node->generic_params, nullptr);
  EXPECT_EQ(vec_get_size(union_node->generic_params), 1);
  ASSERT_NE(union_node->members, nullptr);
  EXPECT_EQ(vec_get_size(union_node->members), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_union, union_generic_multi) {
  const char *source = "union Result[T, E] { value: T; err: E; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_union(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_union_t union_node = (cubec_statement_union_t)node;
  ASSERT_NE(union_node->generic_params, nullptr);
  EXPECT_EQ(vec_get_size(union_node->generic_params), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Export modifier
 * ========================================================================== */

TEST_F(dt_statement_union, export_union) {
  const char *source = "export union Option { value: i32; tag: u64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_union(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_union_t union_node = (cubec_statement_union_t)node;
  EXPECT_TRUE(union_node->is_export);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_union, non_export_union) {
  const char *source = "union Option { value: i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_union(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_union_t union_node = (cubec_statement_union_t)node;
  EXPECT_FALSE(union_node->is_export);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Lifecycle: clone / move
 * ========================================================================== */

TEST_F(dt_statement_union, clone) {
  const char *source = "union Option[T] { value: T; tag: u64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_union(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_UNION);

  cubec_statement_union_t union_node = (cubec_statement_union_t)cloned;
  ASSERT_NE(union_node->name, nullptr);
  ASSERT_NE(union_node->members, nullptr);
  EXPECT_EQ(vec_get_size(union_node->members), 2);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_union, move) {
  const char *source = "union Data { value: i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_union(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_UNION);

  cubec_statement_union_t union_node = (cubec_statement_union_t)moved;
  ASSERT_NE(union_node->name, nullptr);
  ASSERT_NE(union_node->members, nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Token consumption
 * ========================================================================== */

TEST_F(dt_statement_union, consume_all_tokens) {
  const char *source = "union Option { value: i32; tag: u64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_union(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(position, vec_get_size(tokens) - 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Integration
 * ========================================================================== */

TEST_F(dt_statement_union, via_read_statement) {
  const char *source = "union Option { value: i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_UNION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_union, via_read_program) {
  const char *source = "union Option { value: i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_program_node(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_PROGRAM);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  implement clause parsing
 * ========================================================================== */

/* ---- union with single implement ---- */

TEST_F(dt_statement_union, implement_single) {
  const char *source = "union Result[E, T] implement Protocol[E, T] { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_union(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_UNION);

  cubec_statement_union_t union_node = (cubec_statement_union_t)node;
  ASSERT_NE(union_node->implements, nullptr);
  EXPECT_EQ(vec_get_size(union_node->implements), 1u);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- union without implement — implements is NULL ---- */

TEST_F(dt_statement_union, no_implement) {
  const char *source = "union Option[T] { value: T; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_union(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_union_t union_node = (cubec_statement_union_t)node;
  EXPECT_EQ(union_node->implements, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
