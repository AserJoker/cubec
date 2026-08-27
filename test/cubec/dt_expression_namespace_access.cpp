#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/expression.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_member.h"
#include "cubec/expression_subscript.h"
#include "cubec/declaration_pointer.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_expression_namespace_access : public CubecTest {
protected:
};

/* ---- Basic namespace access in normal expressions ---- */

TEST_F(dt_expression_namespace_access, single_namespace_access) {
  const char *source = "std::vec";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(vm, tokens, &position, "test.cubec");
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
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(vm, tokens, &position, "test.cubec");
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
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(vm, tokens, &position, "test.cubec");
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
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);

  cubec_expression_subscript_t generic =
      (cubec_expression_subscript_t)node;
  ASSERT_NE(generic->host, nullptr);
  EXPECT_EQ(generic->host->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Namespace access followed by namespace access (chained ::) ---- */

TEST_F(dt_expression_namespace_access, namespace_then_static_member) {
  /* std::Vec::create() 鈥?namespace navigation then static method access */
  const char *source = "std::Vec::create";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* The outer should be namespace access (::create) 鈥?type member access */
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
  /* a + std::Vec::create() + b 鈥?:: for type member access */
  const char *source = "a + std::Vec::create() + b";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* Should be a binary expression */
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Mixed :: and . : type member (::) then instance member (.) ---- */

TEST_F(dt_expression_namespace_access, type_static_then_instance_member) {
  /* std::Vec::new().field 鈥?:: for type member, . for instance member */
  const char *source = "std::Vec::new().field";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(vm, tokens, &position, "test.cubec");
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
  /* *std::vec::Vec 鈫?pointer to namespaced type */
  const char *source = "* std::vec::Vec";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(vm, tokens, &position, "test.cubec");
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
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* std, ::, vec, ::, Vec, [, i32, ] 鈫?8 tokens */
  EXPECT_EQ(position, 8);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Clone ---- */

TEST_F(dt_expression_namespace_access, clone) {
  const char *source = "std::vec";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
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
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)alloc_move(allocator, node);
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
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_expression(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_expression(ectx, node);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "A::b");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Prefix :: (current-module scope) ---- */

TEST_F(dt_expression_namespace_access, prefix_scope_resolution) {
  /* ::field — current module global scope, host=NULL */
  const char *source = "::global_var";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  cubec_expression_namespace_access_t ns = (cubec_expression_namespace_access_t)node;
  EXPECT_EQ(ns->host, nullptr);
  ASSERT_NE(ns->field, nullptr);
  EXPECT_STREQ(string_get(ns->field->value), "global_var");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_namespace_access, prefix_scope_resolution_then_postfix) {
  /* ::Type::method — prefix :: then chained namespace access */
  const char *source = "::MyType::create";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  cubec_expression_namespace_access_t outer = (cubec_expression_namespace_access_t)node;
  ASSERT_NE(outer->field, nullptr);
  EXPECT_STREQ(string_get(outer->field->value), "create");

  /* host is also a namespace_access with host=NULL */
  ASSERT_NE(outer->host, nullptr);
  EXPECT_EQ(outer->host->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);
  cubec_expression_namespace_access_t inner = (cubec_expression_namespace_access_t)outer->host;
  EXPECT_EQ(inner->host, nullptr);
  EXPECT_STREQ(string_get(inner->field->value), "MyType");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_namespace_access, prefix_scope_resolution_clone) {
  const char *source = "::foo";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  cubec_expression_namespace_access_t ns = (cubec_expression_namespace_access_t)cloned;
  EXPECT_EQ(ns->host, nullptr);
  ASSERT_NE(ns->field, nullptr);
  EXPECT_STREQ(string_get(ns->field->value), "foo");

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_namespace_access, prefix_scope_resolution_move) {
  const char *source = "::bar";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_value(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)alloc_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  cubec_expression_namespace_access_t ns = (cubec_expression_namespace_access_t)moved;
  EXPECT_EQ(ns->host, nullptr);
  ASSERT_NE(ns->field, nullptr);
  EXPECT_STREQ(string_get(ns->field->value), "bar");

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_namespace_access, write_prefix_scope_resolution) {
  const char *source = "::global_var";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_expression(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_expression(ectx, node);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "::global_var");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
