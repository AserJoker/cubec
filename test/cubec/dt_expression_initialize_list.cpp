#include "cubec/expression.h"
#include "cubec/expression_initialize_field.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_initialize_list : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ---- Anonymous empty list: .{} ---- */
TEST_F(dt_expression_initialize_list, anonymous_empty) {
  const char *source = ".{}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t list = (cubec_expression_initialize_list_t)node;
  EXPECT_EQ(list->type, nullptr);
  EXPECT_EQ(vec_get_size(list->items), 0);
  EXPECT_EQ(list->is_field, false);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Typed empty list: .Vec{} ---- */
TEST_F(dt_expression_initialize_list, typed_empty) {
  const char *source = ".Vec{}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t list = (cubec_expression_initialize_list_t)node;
  ASSERT_NE(list->type, nullptr);
  EXPECT_EQ(list->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(vec_get_size(list->items), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Typed with field items: .Vec{.x=1, .y=2} ---- */
TEST_F(dt_expression_initialize_list, typed_field_items) {
  const char *source = ".Vec{.x = 1, .y = 2}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t list = (cubec_expression_initialize_list_t)node;
  ASSERT_NE(list->type, nullptr);
  EXPECT_EQ(list->is_field, true);
  EXPECT_EQ(vec_get_size(list->items), 2);

  /* First field: .x = 1 */
  cubec_expression_initialize_field_t f0 =
      (cubec_expression_initialize_field_t)vec_get(list->items, 0);
  EXPECT_STREQ(string_get(f0->field->value), "x");
  EXPECT_EQ(f0->value->kind, CUBEC_NODE_LITERAL_NUMERIC);

  /* Second field: .y = 2 */
  cubec_expression_initialize_field_t f1 =
      (cubec_expression_initialize_field_t)vec_get(list->items, 1);
  EXPECT_STREQ(string_get(f1->field->value), "y");
  EXPECT_EQ(f1->value->kind, CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Typed with positional items: .Vec{1, 2, 3} ---- */
TEST_F(dt_expression_initialize_list, typed_positional_items) {
  const char *source = ".Vec{1, 2, 3}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t list = (cubec_expression_initialize_list_t)node;
  ASSERT_NE(list->type, nullptr);
  EXPECT_EQ(list->is_field, false);
  EXPECT_EQ(vec_get_size(list->items), 3);

  for (size_t i = 0; i < 3; i++) {
    node_t item = (node_t)vec_get(list->items, i);
    EXPECT_EQ(item->kind, CUBEC_NODE_LITERAL_NUMERIC);
  }

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Anonymous with field items: .{.x=1, .y=2} ---- */
TEST_F(dt_expression_initialize_list, anonymous_field_items) {
  const char *source = ".{.x = 1, .y = 2}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t list = (cubec_expression_initialize_list_t)node;
  EXPECT_EQ(list->type, nullptr);
  EXPECT_EQ(list->is_field, true);
  EXPECT_EQ(vec_get_size(list->items), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Anonymous with positional items: .{1, 2, 3} ---- */
TEST_F(dt_expression_initialize_list, anonymous_positional_items) {
  const char *source = ".{1, 2, 3}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t list = (cubec_expression_initialize_list_t)node;
  EXPECT_EQ(list->type, nullptr);
  EXPECT_EQ(list->is_field, false);
  EXPECT_EQ(vec_get_size(list->items), 3);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Nested initialize_list as expression: .{.Test{}} ---- */
TEST_F(dt_expression_initialize_list, nested_initialize_list_as_expression) {
  const char *source = ".{.Test{}}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t list = (cubec_expression_initialize_list_t)node;
  EXPECT_EQ(list->type, nullptr);
  EXPECT_EQ(list->is_field, false);
  EXPECT_EQ(vec_get_size(list->items), 1);

  /* The item should be an initialize_list, NOT an initialize_field */
  node_t item = (node_t)vec_get(list->items, 0);
  EXPECT_EQ(item->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t inner = (cubec_expression_initialize_list_t)item;
  ASSERT_NE(inner->type, nullptr);
  EXPECT_EQ(inner->type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(vec_get_size(inner->items), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Field vs expression disambiguation: .{.Test=123} ---- */
TEST_F(dt_expression_initialize_list, field_vs_expression_disambiguation) {
  const char *source = ".{.Test = 123}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t list = (cubec_expression_initialize_list_t)node;
  EXPECT_EQ(list->is_field, true);
  EXPECT_EQ(vec_get_size(list->items), 1);

  /* The item should be an initialize_field */
  cubec_expression_initialize_field_t f =
      (cubec_expression_initialize_field_t)vec_get(list->items, 0);
  EXPECT_STREQ(string_get(f->field->value), "Test");
  EXPECT_EQ(f->value->kind, CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Postfix chain: .Vec{1,2}.field ---- */
TEST_F(dt_expression_initialize_list, postfix_member_chain) {
  const char *source = ".Vec{1, 2}.field";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Expression context: 1 + .Vec{1,2} ---- */
TEST_F(dt_expression_initialize_list, in_binary_expression) {
  const char *source = "1 + .Vec{1, 2}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Trailing comma is allowed ---- */
TEST_F(dt_expression_initialize_list, trailing_comma_is_allowed) {
  const char *source = ".Vec{1, }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t list = (cubec_expression_initialize_list_t)node;
  EXPECT_EQ(vec_get_size(list->items), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Error: unclosed brace ---- */
TEST_F(dt_expression_initialize_list, unclosed_brace_is_error) {
  const char *source = ".Vec{1, 2";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Error: mixed field and positional items ---- */
TEST_F(dt_expression_initialize_list, mixed_items_is_error) {
  const char *source = ".Vec{1, .x = 2}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Clone ---- */
TEST_F(dt_expression_initialize_list, clone) {
  const char *source = ".Vec{.x = 1, .y = 2}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t original = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(original, nullptr);

  node_t cloned = (node_t)value_clone(allocator, original);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t list = (cubec_expression_initialize_list_t)cloned;
  ASSERT_NE(list->type, nullptr);
  EXPECT_EQ(list->is_field, true);
  EXPECT_EQ(vec_get_size(list->items), 2);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &original);
  allocator_free(allocator, &tokens);
}

/* ---- Move ---- */
TEST_F(dt_expression_initialize_list, move) {
  const char *source = ".Vec{1, 2}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t original = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(original, nullptr);

  node_t moved = (node_t)value_move(allocator, original);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &original);
  allocator_free(allocator, &tokens);
}

/* ---- Single positional item ---- */
TEST_F(dt_expression_initialize_list, single_positional_item) {
  const char *source = ".Vec{42}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t list = (cubec_expression_initialize_list_t)node;
  EXPECT_EQ(list->is_field, false);
  EXPECT_EQ(vec_get_size(list->items), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Single field item ---- */
TEST_F(dt_expression_initialize_list, single_field_item) {
  const char *source = ".Vec{.x = 1}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t list = (cubec_expression_initialize_list_t)node;
  EXPECT_EQ(list->is_field, true);
  EXPECT_EQ(vec_get_size(list->items), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Not an initialize_list: no dot ---- */
TEST_F(dt_expression_initialize_list, no_dot_returns_null) {
  const char *source = "Vec{1, 2}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  /* Should not parse as initialize_list; read_expression will try other paths */
  node_t node = read_expression_initialize_list(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Not an initialize_list: dot + identifier without brace ---- */
TEST_F(dt_expression_initialize_list, dot_identifier_no_brace_returns_null) {
  const char *source = ".Vec";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_initialize_list(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Anonymous with nested typed initialize_list and field in same list ---- */
TEST_F(dt_expression_initialize_list, nested_typed_with_fields) {
  const char *source = ".{.Inner{.a = 1}}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t outer = (cubec_expression_initialize_list_t)node;
  EXPECT_EQ(outer->type, nullptr);
  EXPECT_EQ(outer->is_field, false);
  EXPECT_EQ(vec_get_size(outer->items), 1);

  /* Outer item is the inner initialize_list expression */
  node_t inner_item = (node_t)vec_get(outer->items, 0);
  EXPECT_EQ(inner_item->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t inner = (cubec_expression_initialize_list_t)inner_item;
  ASSERT_NE(inner->type, nullptr);
  EXPECT_EQ(inner->is_field, true);
  EXPECT_EQ(vec_get_size(inner->items), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Typed with member access type: .std.vec.Vec{1, 2} ---- */
TEST_F(dt_expression_initialize_list, typed_namespace_access_type) {
  const char *source = ".std::vec::Vec{1, 2}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t list = (cubec_expression_initialize_list_t)node;
  ASSERT_NE(list->type, nullptr);
  /* Type should be a namespace access expression */
  EXPECT_EQ(list->type->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);
  EXPECT_EQ(list->is_field, false);
  EXPECT_EQ(vec_get_size(list->items), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Typed with generic instantiation: .Vec[i32]{1, 2} ---- */
TEST_F(dt_expression_initialize_list, typed_generic_instantiation) {
  const char *source = ".Vec[i32]{1, 2}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t list = (cubec_expression_initialize_list_t)node;
  ASSERT_NE(list->type, nullptr);
  /* Type should be a generic instantiation expression */
  EXPECT_EQ(list->type->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);
  EXPECT_EQ(list->is_field, false);
  EXPECT_EQ(vec_get_size(list->items), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Typed with pointer type: .*i32{1, 2} ---- */
TEST_F(dt_expression_initialize_list, typed_pointer_type) {
  const char *source = ".* i32{1, 2}";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_INITIALIZE_LIST);

  cubec_expression_initialize_list_t list = (cubec_expression_initialize_list_t)node;
  ASSERT_NE(list->type, nullptr);
  /* Type should be a pointer declaration */
  EXPECT_EQ(list->type->kind, CUBEC_NODE_DECLARATION_POINTER);
  EXPECT_EQ(list->is_field, false);
  EXPECT_EQ(vec_get_size(list->items), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
