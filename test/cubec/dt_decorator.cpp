#include "cubec/decorator.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "core/string.h"
#include "core/token_writer.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_decorator : public CubecTest {
protected:
};

TEST_F(dt_decorator, simple_identifier) {
  const char *source = "[[inline]]";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_decorator(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECORATOR);

  cubec_decorator_t dec = (cubec_decorator_t)node;
  ASSERT_NE(dec->expression, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_decorator, call_expression) {
  const char *source = "[[deprecated(\"use new_api instead\")]]";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_decorator(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECORATOR);

  cubec_decorator_t dec = (cubec_decorator_t)node;
  ASSERT_NE(dec->expression, nullptr);
  EXPECT_EQ(dec->expression->kind, CUBEC_NODE_EXPRESSION_CALL);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_decorator, export_decorator) {
  const char *source = "[[export]]";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_decorator(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_decorator, clone) {
  const char *source = "[[inline]]";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_decorator(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_DECORATOR);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_decorator, move) {
  const char *source = "[[inline]]";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_decorator(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)alloc_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_DECORATOR);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_decorator, non_decorator_returns_null) {
  const char *source = "var x = 1;";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_decorator(vm, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_decorator, single_bracket_not_decorator) {
  const char *source = "[1, 2]";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_decorator(vm, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_decorator, write_simple_identifier) {
  const char *source = "[[test]]";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_decorator(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_decorator(ectx, node);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "[[test]]");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_decorator, write_call_decorator) {
  const char *source = R"([[deprecated("use new_api instead")]])";
  vec_t tokens = resolve_token_list(vm, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_decorator(vm, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_decorator(ectx, node);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, R"([[deprecated("use new_api instead")]])");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
