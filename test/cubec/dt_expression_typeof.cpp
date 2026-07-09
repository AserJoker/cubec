#include "cubec/expression_typeof.h"
#include "cubec/expression.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_typeof : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* ---- Basic typeof with identifier ---- */

TEST_F(dt_expression_typeof, typeof_identifier) {
  const char *source = "typeof(x)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_typeof(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPEOF);

  cubec_expression_typeof_t t = (cubec_expression_typeof_t)node;
  ASSERT_NE(t->expression, nullptr);
  EXPECT_EQ(t->expression->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- typeof with binary expression ---- */

TEST_F(dt_expression_typeof, typeof_binary_expression) {
  const char *source = "typeof(a + b)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_typeof(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPEOF);

  cubec_expression_typeof_t t = (cubec_expression_typeof_t)node;
  ASSERT_NE(t->expression, nullptr);
  EXPECT_EQ(t->expression->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- typeof with function call ---- */

TEST_F(dt_expression_typeof, typeof_function_call) {
  const char *source = "typeof(foo())";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_typeof(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPEOF);

  cubec_expression_typeof_t t = (cubec_expression_typeof_t)node;
  ASSERT_NE(t->expression, nullptr);
  EXPECT_EQ(t->expression->kind, CUBEC_NODE_EXPRESSION_CALL);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- typeof is a type: usable in type expression context ---- */

TEST_F(dt_expression_typeof, typeof_as_type_expression) {
  const char *source = "typeof(x)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPEOF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- typeof with namespace access: typeof(File)::open ---- */

TEST_F(dt_expression_typeof, typeof_with_namespace_access) {
  const char *source = "typeof(File)::open";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  cubec_expression_namespace_access_t ns = (cubec_expression_namespace_access_t)node;
  ASSERT_NE(ns->host, nullptr);
  EXPECT_EQ(ns->host->kind, CUBEC_NODE_EXPRESSION_TYPEOF);
  EXPECT_STREQ(string_get(ns->field->value), "open");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- typeof is NOT a pointer base: * typeof(x) fails to parse as type expr ---- */

TEST_F(dt_expression_typeof, typeof_not_pointer_base) {
  const char *source = "* typeof(x)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = NULL;
  /* Since typeof is a top-level type expression (not in primary), the pointer
   * parser in read_type_expression_primary consumes '*' but cannot find a
   * valid inner type ('typeof' is a keyword, not an identifier). The parse
   * fails — typeof(x) cannot be wrapped by a pointer declaration. */
  CATCH_ERROR(
      node = read_expression_type(allocator, tokens, &position, "test.cubec"),
      error_clear());
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- typeof is NOT a slice base: [] typeof(x) fails to parse as type expr ---- */

TEST_F(dt_expression_typeof, typeof_not_slice_base) {
  const char *source = "[] typeof(x)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = NULL;
  /* Since typeof is a top-level type expression (not in primary), the slice
   * parser in read_type_expression_primary consumes '[]' but cannot find a
   * valid inner type. The parse fails — typeof(x) cannot be wrapped by a
   * slice declaration. */
  CATCH_ERROR(
      node = read_expression_type(allocator, tokens, &position, "test.cubec"),
      error_clear());
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- typeof with generic instantiation: typeof(Vec)[i32] ---- */

TEST_F(dt_expression_typeof, typeof_with_generic_instantiation) {
  const char *source = "typeof(Vec)[i32]";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  cubec_expression_generic_instantiation_t generic =
      (cubec_expression_generic_instantiation_t)node;
  ASSERT_NE(generic->callee, nullptr);
  EXPECT_EQ(generic->callee->kind, CUBEC_NODE_EXPRESSION_TYPEOF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Consume all tokens ---- */

TEST_F(dt_expression_typeof, consume_all_tokens) {
  const char *source = "typeof(x)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_typeof(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* typeof, (, x, ) → 4 tokens */
  EXPECT_EQ(position, 4);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Error: missing opening parenthesis ---- */

TEST_F(dt_expression_typeof, missing_lparen_is_error) {
  const char *source = "typeof x";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = NULL;
  CATCH_ERROR(
      node = read_expression_typeof(allocator, tokens, &position, "test.cubec"),
      error_clear());
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Error: missing closing parenthesis ---- */

TEST_F(dt_expression_typeof, missing_rparen_is_error) {
  const char *source = "typeof(x";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = NULL;
  CATCH_ERROR(
      node = read_expression_typeof(allocator, tokens, &position, "test.cubec"),
      error_clear());
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Not typeof: regular identifier ---- */

TEST_F(dt_expression_typeof, not_typeof_returns_null) {
  const char *source = "foo";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_typeof(allocator, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Clone ---- */

TEST_F(dt_expression_typeof, clone) {
  const char *source = "typeof(x)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_typeof(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_EXPRESSION_TYPEOF);

  cubec_expression_typeof_t t = (cubec_expression_typeof_t)cloned;
  ASSERT_NE(t->expression, nullptr);
  EXPECT_EQ(t->expression->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Move ---- */

TEST_F(dt_expression_typeof, move) {
  const char *source = "typeof(x)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_typeof(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_EXPRESSION_TYPEOF);

  cubec_expression_typeof_t t = (cubec_expression_typeof_t)moved;
  ASSERT_NE(t->expression, nullptr);
  EXPECT_EQ(t->expression->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
