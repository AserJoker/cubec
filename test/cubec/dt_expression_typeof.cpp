#include "cubec/expression_typeof.h"
#include "cubec/expression.h"
#include "cubec/expression_call.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_member.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/declaration_slice.h"
#include "cubec/declaration_pointer.h"
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

TEST_F(dt_expression_typeof, typeof_as_pointer_base) {
  const char *source = "* typeof(x)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_EXPRESSION_TYPEOF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- typeof is NOT a slice base: [] typeof(x) fails to parse as type expr ---- */

TEST_F(dt_expression_typeof, typeof_as_slice_base) {
  const char *source = "[] typeof(x)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_SLICE);

  cubec_declaration_slice_t slice = (cubec_declaration_slice_t)node;
  ASSERT_NE(slice->type, nullptr);
  EXPECT_EQ(slice->type->kind, CUBEC_NODE_EXPRESSION_TYPEOF);

  allocator_free(allocator, &node);
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

/* ==========================================================================
 *  typeof in expression context (via read_expression)
 * ========================================================================== */

/* ---- typeof via read_expression ---- */

TEST_F(dt_expression_typeof, via_read_expression) {
  const char *source = "typeof(x)";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TYPEOF);

  cubec_expression_typeof_t t = (cubec_expression_typeof_t)node;
  ASSERT_NE(t->expression, nullptr);
  EXPECT_EQ(t->expression->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- typeof with member access in expression: typeof(x).field ---- */

TEST_F(dt_expression_typeof, typeof_member_access_in_expression) {
  const char *source = "typeof(x).field";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t mem = (cubec_expression_member_t)node;
  ASSERT_NE(mem->host, nullptr);
  EXPECT_EQ(mem->host->kind, CUBEC_NODE_EXPRESSION_TYPEOF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- typeof with call in expression: typeof(x)() ---- */

TEST_F(dt_expression_typeof, typeof_call_in_expression) {
  const char *source = "typeof(x)()";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_CALL);

  cubec_expression_call_t call = (cubec_expression_call_t)node;
  ASSERT_NE(call->callee, nullptr);
  EXPECT_EQ(call->callee->kind, CUBEC_NODE_EXPRESSION_TYPEOF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- typeof with namespace access in expression: typeof(x)::method ---- */

TEST_F(dt_expression_typeof, typeof_namespace_access_in_expression) {
  const char *source = "typeof(x)::method";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  cubec_expression_namespace_access_t ns = (cubec_expression_namespace_access_t)node;
  ASSERT_NE(ns->host, nullptr);
  EXPECT_EQ(ns->host->kind, CUBEC_NODE_EXPRESSION_TYPEOF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- typeof in ternary: typeof(x) == i32 ? a : b ---- */

TEST_F(dt_expression_typeof, typeof_in_ternary) {
  const char *source = "typeof(x) == i32 ? a : b";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
