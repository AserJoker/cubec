#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/statement_switch.h"
#include "cubec/switch_match.h"
#include "cubec/node.h"
#include "cubec/statement.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_statement_switch : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

TEST_F(dt_statement_switch, simple_switch) {
  const char *source = "switch(x) { case(1) -> { } case(2) -> { } else -> { } }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_switch(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_SWITCH);

  cubec_statement_switch_t sw = (cubec_statement_switch_t)node;
  ASSERT_NE(sw->condition, nullptr);
  ASSERT_NE(sw->matches, nullptr);
  EXPECT_EQ(vec_get_size(sw->matches), 3);

  /* First match: case(1) */
  cubec_switch_match_t m0 = (cubec_switch_match_t)vec_get(sw->matches, 0);
  EXPECT_FALSE(m0->is_else);
  ASSERT_NE(m0->values, nullptr);
  EXPECT_EQ(vec_get_size(m0->values), 1);
  ASSERT_NE(m0->body, nullptr);

  /* Second match: case(2) */
  cubec_switch_match_t m1 = (cubec_switch_match_t)vec_get(sw->matches, 1);
  EXPECT_FALSE(m1->is_else);
  ASSERT_NE(m1->values, nullptr);
  EXPECT_EQ(vec_get_size(m1->values), 1);

  /* Third match: else */
  cubec_switch_match_t m2 = (cubec_switch_match_t)vec_get(sw->matches, 2);
  EXPECT_TRUE(m2->is_else);
  EXPECT_EQ(m2->values, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_switch, multi_value_case) {
  const char *source = "switch(x) { case(1, 2, 3) -> { } else -> { } }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_switch(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_switch_t sw = (cubec_statement_switch_t)node;
  cubec_switch_match_t m0 = (cubec_switch_match_t)vec_get(sw->matches, 0);
  EXPECT_FALSE(m0->is_else);
  EXPECT_EQ(vec_get_size(m0->values), 3);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_switch, switch_no_else) {
  const char *source = "switch(x) { case(1) -> { } case(2) -> { } }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_switch(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_switch_t sw = (cubec_statement_switch_t)node;
  EXPECT_EQ(vec_get_size(sw->matches), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_switch, switch_only_else) {
  const char *source = "switch(x) { else -> { } }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_switch(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_switch_t sw = (cubec_statement_switch_t)node;
  EXPECT_EQ(vec_get_size(sw->matches), 1);
  cubec_switch_match_t m0 = (cubec_switch_match_t)vec_get(sw->matches, 0);
  EXPECT_TRUE(m0->is_else);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_switch, switch_empty) {
  const char *source = "switch(x) { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_switch(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_switch_t sw = (cubec_statement_switch_t)node;
  EXPECT_EQ(vec_get_size(sw->matches), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_switch, clone) {
  const char *source = "switch(x) { case(1) -> { } else -> { } }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_switch(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_SWITCH);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_switch, move) {
  const char *source = "switch(x) { case(1) -> { } else -> { } }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_switch(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_SWITCH);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_switch, via_read_statement) {
  const char *source = "switch(x) { case(1) -> { } else -> { } }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_SWITCH);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_switch, non_switch_returns_null) {
  const char *source = "if(x) { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_switch(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_switch, case_with_expression) {
  const char *source = "switch(x) { case(1 + 1) -> { } }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_switch(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_switch_t sw = (cubec_statement_switch_t)node;
  cubec_switch_match_t m0 = (cubec_switch_match_t)vec_get(sw->matches, 0);
  EXPECT_EQ(vec_get_size(m0->values), 1);
  EXPECT_EQ(((node_t)vec_get(m0->values, 0))->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Write round-trip tests
 * ========================================================================== */

TEST_F(dt_statement_switch, write_empty_switch) {
  const char *source = "switch(x) { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_statement(ectx, node);
  emit_newline(ectx);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "switch (x) {\n}\n");
  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
