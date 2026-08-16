#include "common/test_common.h"
#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/expression.h"
#include "cubec/expression_try.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_expression_try : public CubecTest {
protected:
};

TEST_F(dt_expression_try, parse_try) {
  const char *source = "x.?";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_EXPRESSION_TRY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_expression_try, write_basic) {
  const char *source = "x.?";
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
  EXPECT_STREQ(output, "x.?");

  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
