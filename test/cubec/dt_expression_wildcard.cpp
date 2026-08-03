#include "common/test_common.h"
#include "core/string.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "cubec/expression_wildcard.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_expression_wildcard : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

TEST_F(dt_expression_wildcard, parse_simple) {
  const char *source = "?";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_wildcard(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_WILDCARD);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_wildcard, parse_tuple) {
  const char *source = "<?>";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_WILDCARD);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_wildcard, write_simple) {
  const char *source = "?";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_expression_wildcard(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  writer_t writer = (writer_t)allocator_create(allocator, &g_writer_type, NULL);
  write_expression(writer, node);
  const char *output = string_get(writer_get_string(writer));
  EXPECT_STREQ(output, "?");

  allocator_free(allocator, &writer);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_wildcard, write_tuple) {
  const char *source = "<?>";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  writer_t writer = (writer_t)allocator_create(allocator, &g_writer_type, NULL);
  write_expression(writer, node);
  const char *output = string_get(writer_get_string(writer));
  EXPECT_STREQ(output, "<?>");

  allocator_free(allocator, &writer);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
