#include "core/string.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_member.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/declaration_pointer.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_namespace_access : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ---- Basic namespace access in normal expressions ---- */

TEST_F(dt_expression_namespace_access, single_namespace_access) {
  const char *source = "std::vec";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  cubec_expression_namespace_access_t ns = (cubec_expression_namespace_access_t)node;
  ASSERT_NE(ns->host, nullptr);
  EXPECT_EQ(ns->host->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(string_get(((cubec_literal_identifier_t)ns->host)->value), "std");

  ASSERT_NE(ns->field, nullptr);
  EXPECT_STREQ(string_get(ns->field->value), "vec");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_namespace_access, chained_namespace_access) {
  const char *source = "std::vec::Vec";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  /* Outer: (std::vec)::Vec */
  cubec_expression_namespace_access_t outer = (cubec_expression_namespace_access_t)node;
  ASSERT_NE(outer->field, nullptr);
  EXPECT_STREQ(string_get(outer->field->value), "Vec");

  ASSERT_NE(outer->host, nullptr);
  EXPECT_EQ(outer->host->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);
  cubec_expression_namespace_access_t inner = (cubec_expression_namespace_access_t)outer->host;
  EXPECT_STREQ(string_get(inner->field->value), "vec");

  ASSERT_NE(inner->host, nullptr);
  EXPECT_EQ(inner->host->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(string_get(((cubec_literal_identifier_t)inner->host)->value), "std");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Namespace access with spaces ---- */

TEST_F(dt_expression_namespace_access, namespace_access_with_spaces) {
  const char *source = "std :: vec";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  cubec_expression_namespace_access_t ns = (cubec_expression_namespace_access_t)node;
  EXPECT_STREQ(string_get(((cubec_literal_identifier_t)ns->host)->value), "std");
  EXPECT_STREQ(string_get(ns->field->value), "vec");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Namespace access followed by generic instantiation ---- */

TEST_F(dt_expression_namespace_access, namespace_with_generic) {
  const char *source = "std::vec::Vec[i32]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  cubec_expression_generic_instantiation_t generic =
      (cubec_expression_generic_instantiation_t)node;
  ASSERT_NE(generic->callee, nullptr);
  EXPECT_EQ(generic->callee->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Namespace access followed by namespace access (chained ::) ---- */

TEST_F(dt_expression_namespace_access, namespace_then_static_member) {
  /* std::Vec::create() — namespace navigation then static method access */
  const char *source = "std::Vec::create";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* The outer should be namespace access (::create) — type member access */
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  cubec_expression_namespace_access_t ns = (cubec_expression_namespace_access_t)node;
  /* Host should be namespace access (std::Vec) */
  ASSERT_NE(ns->host, nullptr);
  EXPECT_EQ(ns->host->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);
  EXPECT_STREQ(string_get(ns->field->value), "create");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Namespace access in normal expression context ---- */

TEST_F(dt_expression_namespace_access, namespace_in_expression) {
  /* a + std::Vec::create() + b — :: for type member access */
  const char *source = "a + std::Vec::create() + b";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* Should be a binary expression */
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Mixed :: and . : type member (::) then instance member (.) ---- */

TEST_F(dt_expression_namespace_access, type_static_then_instance_member) {
  /* std::Vec::new().field — :: for type member, . for instance member */
  const char *source = "std::Vec::new().field";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* Outer: instance member access (.field) */
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t member = (cubec_expression_member_t)node;
  EXPECT_STREQ(string_get(member->field->value), "field");

  /* Host: call(std::Vec::new, args) */
  ASSERT_NE(member->host, nullptr);
  EXPECT_EQ(member->host->kind, CUBEC_NODE_EXPRESSION_CALL);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Namespace access in type expression context ---- */

TEST_F(dt_expression_namespace_access, namespace_in_type_expression) {
  /* *std::vec::Vec → pointer to namespaced type */
  const char *source = "* std::vec::Vec";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Consume all tokens ---- */

TEST_F(dt_expression_namespace_access, consume_all_tokens) {
  const char *source = "std::vec::Vec[i32]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* std, ::, vec, ::, Vec, [, i32, ] → 8 tokens */
  EXPECT_EQ(position, 8);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Clone ---- */

TEST_F(dt_expression_namespace_access, clone) {
  const char *source = "std::vec";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  cubec_expression_namespace_access_t ns = (cubec_expression_namespace_access_t)cloned;
  ASSERT_NE(ns->host, nullptr);
  EXPECT_STREQ(string_get(((cubec_literal_identifier_t)ns->host)->value), "std");
  ASSERT_NE(ns->field, nullptr);
  EXPECT_STREQ(string_get(ns->field->value), "vec");

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Move ---- */

TEST_F(dt_expression_namespace_access, move) {
  const char *source = "std::vec";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  cubec_expression_namespace_access_t ns = (cubec_expression_namespace_access_t)moved;
  ASSERT_NE(ns->host, nullptr);
  EXPECT_STREQ(string_get(((cubec_literal_identifier_t)ns->host)->value), "std");

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_namespace_access, write_namespace_access) {
  const char *source = "A::b";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  writer_t writer = (writer_t)allocator_create(allocator, &g_writer_type, NULL);
  write_expression(writer, node);
  string_t result = writer_get_string(writer); const char *output = string_get(result);
  EXPECT_STREQ(output, "A::b");

  allocator_free(allocator, &result); allocator_free(allocator, &writer);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
