#include "cubec/statement_cunion.h"
#include "cubec/struct_field.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/program.h"
#include "cubec/statement.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_statement_cunion : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* ==========================================================================
 *  Basic parsing
 * ========================================================================== */

TEST_F(dt_statement_cunion, basic_cunion) {
  const char *source = "cunion Data { int_val: i32; float_val: f64; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_cunion(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_CUNION);

  cubec_statement_cunion_t cunion_node = (cubec_statement_cunion_t)node;
  ASSERT_NE(cunion_node->name, nullptr);
  EXPECT_EQ(cunion_node->name->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(cunion_node->fields, nullptr);
  EXPECT_EQ(vec_get_size(cunion_node->fields), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Empty cunion ---- */

TEST_F(dt_statement_cunion, cunion_empty) {
  const char *source = "cunion Empty { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_cunion(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_CUNION);

  cubec_statement_cunion_t cunion_node = (cubec_statement_cunion_t)node;
  ASSERT_NE(cunion_node->fields, nullptr);
  EXPECT_EQ(vec_get_size(cunion_node->fields), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- cunion with pointer field ---- */

TEST_F(dt_statement_cunion, cunion_with_pointer_field) {
  const char *source = "cunion Value { as_i32: i32; as_ptr: *u8; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_cunion(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_cunion_t cunion_node = (cubec_statement_cunion_t)node;
  ASSERT_NE(cunion_node->fields, nullptr);
  EXPECT_EQ(vec_get_size(cunion_node->fields), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- cunion field details ---- */

TEST_F(dt_statement_cunion, cunion_field_details) {
  const char *source = "cunion Data { int_val: i32; float_val: f64; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_cunion(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_cunion_t cunion_node = (cubec_statement_cunion_t)node;
  ASSERT_NE(cunion_node->fields, nullptr);
  EXPECT_EQ(vec_get_size(cunion_node->fields), 2);

  /* First field */
  node_t field0 = (node_t)vec_get(cunion_node->fields, 0);
  EXPECT_EQ(field0->kind, CUBEC_NODE_STRUCT_FIELD);
  cubec_struct_field_t field = (cubec_struct_field_t)field0;
  EXPECT_FALSE(field->is_pub);
  ASSERT_NE(field->name, nullptr);
  ASSERT_NE(field->type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Lifecycle: clone / move
 * ========================================================================== */

TEST_F(dt_statement_cunion, clone) {
  const char *source = "cunion Data { int_val: i32; float_val: f64; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_cunion(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_CUNION);

  cubec_statement_cunion_t cunion_node = (cubec_statement_cunion_t)cloned;
  ASSERT_NE(cunion_node->name, nullptr);
  ASSERT_NE(cunion_node->fields, nullptr);
  EXPECT_EQ(vec_get_size(cunion_node->fields), 2);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_cunion, move) {
  const char *source = "cunion Data { int_val: i32; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_cunion(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_CUNION);

  cubec_statement_cunion_t cunion_node = (cubec_statement_cunion_t)moved;
  ASSERT_NE(cunion_node->name, nullptr);
  ASSERT_NE(cunion_node->fields, nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Token consumption
 * ========================================================================== */

TEST_F(dt_statement_cunion, consume_all_tokens) {
  const char *source = "cunion Data { int_val: i32; float_val: f64; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_cunion(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(position, vec_get_size(tokens) - 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Integration
 * ========================================================================== */

TEST_F(dt_statement_cunion, via_read_statement) {
  const char *source = "cunion Data { int_val: i32; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_CUNION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_cunion, via_read_program) {
  const char *source = "cunion Data { int_val: i32; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_program_node(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_PROGRAM);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Non-cunion returns NULL
 * ========================================================================== */

TEST_F(dt_statement_cunion, non_cunion_returns_null) {
  const char *source = "struct Foo { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_cunion(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}
