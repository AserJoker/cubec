#include "common/test_common.h"
#include "core/string.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "cubec/function_argument.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_function_argument : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

TEST_F(dt_function_argument, parse_name_only) {
  const char *source = "a";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_function_argument(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_FUNCTION_ARGUMENT);

  cubec_function_argument_t arg = (cubec_function_argument_t)node;
  ASSERT_NE(arg->identifier, nullptr);
  cubec_literal_identifier_t id = (cubec_literal_identifier_t)arg->identifier;
  EXPECT_STREQ(string_get(id->value), "a");
  EXPECT_EQ(arg->type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_function_argument, parse_name_with_type) {
  const char *source = "a: i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_function_argument(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_FUNCTION_ARGUMENT);

  cubec_function_argument_t arg = (cubec_function_argument_t)node;
  ASSERT_NE(arg->identifier, nullptr);
  cubec_literal_identifier_t id = (cubec_literal_identifier_t)arg->identifier;
  EXPECT_STREQ(string_get(id->value), "a");
  ASSERT_NE(arg->type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_function_argument, non_identifier_returns_null) {
  const char *source = "123";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_function_argument(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_function_argument, write_name_only) {
  const char *source = "a";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_function_argument(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  writer_t writer = (writer_t)allocator_create(allocator, &g_writer_type, NULL);
  write_function_argument(writer, node);
  const char *output = string_get(writer_get_string(writer));
  EXPECT_STREQ(output, "a");

  allocator_free(allocator, &writer);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_function_argument, write_name_with_type) {
  const char *source = "a: i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_function_argument(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  writer_t writer = (writer_t)allocator_create(allocator, &g_writer_type, NULL);
  write_function_argument(writer, node);
  const char *output = string_get(writer_get_string(writer));
  EXPECT_STREQ(output, "a: i32");

  allocator_free(allocator, &writer);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
