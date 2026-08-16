#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/expression.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_call.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_member.h"
#include "cubec/expression_slice.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_expression_slice : public CubecTest {
protected:
  void SetUp() override { CubecTest::SetUp(); }
};

/* --------------------------------------------------------------------------
 *  Basic slice: arr[start:length]
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_slice, simple_with_start_and_length) {
  const char *source = "arr[0:10]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SLICE);

  cubec_expression_slice_t slice = (cubec_expression_slice_t)node;
  ASSERT_NE(slice->host, nullptr);
  EXPECT_EQ(slice->host->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)slice->host)->value), "arr");

  ASSERT_NE(slice->start, nullptr);
  EXPECT_EQ(slice->start->kind, CUBEC_NODE_LITERAL_NUMERIC);

  ASSERT_NE(slice->length, nullptr);
  EXPECT_EQ(slice->length->kind, CUBEC_NODE_LITERAL_NUMERIC);

  /* arr, [, 0, :, 10, ] */
  EXPECT_EQ(position, 6);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_slice, simple_with_start_only) {
  const char *source = "arr[5:]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SLICE);

  cubec_expression_slice_t slice = (cubec_expression_slice_t)node;
  ASSERT_NE(slice->start, nullptr);
  EXPECT_EQ(slice->start->kind, CUBEC_NODE_LITERAL_NUMERIC);

  /* length is NULL when omitted */
  EXPECT_EQ(slice->length, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_slice, simple_with_length_only) {
  const char *source = "arr[:10]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SLICE);

  cubec_expression_slice_t slice = (cubec_expression_slice_t)node;
  /* start is NULL when omitted */
  EXPECT_EQ(slice->start, nullptr);

  ASSERT_NE(slice->length, nullptr);
  EXPECT_EQ(slice->length->kind, CUBEC_NODE_LITERAL_NUMERIC);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_slice, with_identifier_index) {
  const char *source = "arr[start:len]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SLICE);

  cubec_expression_slice_t slice = (cubec_expression_slice_t)node;
  ASSERT_NE(slice->start, nullptr);
  EXPECT_EQ(slice->start->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)slice->start)->value), "start");

  ASSERT_NE(slice->length, nullptr);
  EXPECT_EQ(slice->length->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_STREQ(
      string_get(((cubec_literal_identifier_t)slice->length)->value), "len");

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_slice, with_binary_expression) {
  const char *source = "arr[a + 1:b - 1]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SLICE);

  cubec_expression_slice_t slice = (cubec_expression_slice_t)node;
  ASSERT_NE(slice->start, nullptr);
  EXPECT_EQ(slice->start->kind, CUBEC_NODE_EXPRESSION_BINARY);

  ASSERT_NE(slice->length, nullptr);
  EXPECT_EQ(slice->length->kind, CUBEC_NODE_EXPRESSION_BINARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Slice on different host expressions
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_slice, slice_on_call_result) {
  /* getArray()[0:10] 鈥?slice on call result */
  const char *source = "getArray()[0:10]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SLICE);

  cubec_expression_slice_t slice = (cubec_expression_slice_t)node;
  EXPECT_EQ(slice->host->kind, CUBEC_NODE_EXPRESSION_CALL);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_slice, slice_on_member_access) {
  /* obj.arr[0:10] 鈥?slice on member access */
  const char *source = "obj.arr[0:10]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SLICE);

  cubec_expression_slice_t slice = (cubec_expression_slice_t)node;
  EXPECT_EQ(slice->host->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Chaining: slice combined with call and member
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_slice, slice_then_member) {
  /* arr[0:10].field 鈥?slice then member access */
  const char *source = "arr[0:10].field";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_MEMBER);

  cubec_expression_member_t member = (cubec_expression_member_t)node;
  EXPECT_EQ(member->host->kind, CUBEC_NODE_EXPRESSION_SLICE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_slice, slice_then_call) {
  /* arr[0:10]() 鈥?slice then call (unlikely but should work) */
  const char *source = "arr[0:10]()";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_CALL);

  cubec_expression_call_t call = (cubec_expression_call_t)node;
  EXPECT_EQ(call->callee->kind, CUBEC_NODE_EXPRESSION_SLICE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_slice, chained_slices) {
  /* arr[1:2][0:1] 鈥?nested slice */
  const char *source = "arr[1:2][0:1]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SLICE);

  cubec_expression_slice_t outer = (cubec_expression_slice_t)node;
  EXPECT_EQ(outer->host->kind, CUBEC_NODE_EXPRESSION_SLICE);

  cubec_expression_slice_t inner = (cubec_expression_slice_t)outer->host;
  EXPECT_EQ(inner->host->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Error / negative cases
 * -------------------------------------------------------------------------- */

TEST_F(dt_expression_slice, not_a_slice_no_bracket) {
  const char *source = "arr";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* Should just be an identifier, not a slice */
  EXPECT_EQ(node->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_slice, empty_brackets_is_generic) {
  /* arr[] without ':' is valid generic instantiation with empty args,
   * NOT a slice error. Slice requires ':' to distinguish from generics. */
  const char *source = "arr[]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* Should be generic_instantiation with empty args, not slice */
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  cubec_expression_generic_instantiation_t gi =
      (cubec_expression_generic_instantiation_t)node;
  EXPECT_EQ(vec_get_size(gi->arguments), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_slice, missing_colon_error) {
  const char *source = "arr[0]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* Should be generic_instantiation, not slice */
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_slice, unclosed_bracket) {
  const char *source = "arr[0:10";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  /* parse error expected: unclosed bracket 鈫?recorded in diagnostics */
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  EXPECT_GT(context_get_error_count(ctx), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_slice, start_without_colon_error) {
  const char *source = "arr[0 10]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  /* parse error expected: expected ':' after start expression 鈫?recorded in diagnostics */
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  EXPECT_GT(context_get_error_count(ctx), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Complex expressions in slice indices
 * ---------------------------------------------------------------------------------- */

TEST_F(dt_expression_slice, complex_start_expression) {
  const char *source = "arr[a * 2 + 1:len]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SLICE);

  cubec_expression_slice_t slice = (cubec_expression_slice_t)node;
  EXPECT_EQ(slice->start->kind, CUBEC_NODE_EXPRESSION_BINARY);
  EXPECT_EQ(slice->length->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_slice, nested_ternary_in_slice) {
  /* arr[a ? b : c: len] 鈥?ternary in start position */
  const char *source = "arr[a ? b : c: len]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_SLICE);

  cubec_expression_slice_t slice = (cubec_expression_slice_t)node;
  EXPECT_EQ(slice->start->kind, CUBEC_NODE_EXPRESSION_TERNARY);
  EXPECT_EQ(slice->length->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
TEST_F(dt_expression_slice, write_slice_full) {
  const char *source = "a[1:3]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_expression(ectx, node);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "a[1:3]");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_slice, write_slice_no_length) {
  const char *source = "a[1:]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_expression(ectx, node);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "a[1:]");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_slice, write_slice_no_start) {
  const char *source = "a[:3]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_expression(ectx, node);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "a[:3]");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
