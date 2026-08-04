#include "common/test_common.h"
#include "core/string.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "cubec/generic_param.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_generic_param : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

TEST_F(dt_generic_param, parse_simple) {
  const char *source = "[T]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  vec_t params = read_generic_params(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(params, nullptr);
  EXPECT_EQ(vec_get_size(params), 1);

  node_t param = (node_t)vec_get(params, 0);
  EXPECT_EQ(param->kind, CUBEC_NODE_GENERIC_PARAM);

  cubec_generic_param_t gp = (cubec_generic_param_t)param;
  ASSERT_NE(gp->name, nullptr);
  cubec_literal_identifier_t name = (cubec_literal_identifier_t)gp->name;
  EXPECT_STREQ(string_get(name->value), "T");
  EXPECT_EQ(gp->constraints, nullptr);
  EXPECT_EQ(gp->value_type, nullptr);
  EXPECT_FALSE(gp->is_rest);

  allocator_free(allocator, &params);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_generic_param, parse_with_constraint) {
  const char *source = "[T extends Hashable]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  vec_t params = read_generic_params(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(params, nullptr);
  EXPECT_EQ(vec_get_size(params), 1);

  node_t param = (node_t)vec_get(params, 0);
  cubec_generic_param_t gp = (cubec_generic_param_t)param;
  ASSERT_NE(gp->name, nullptr);
  cubec_literal_identifier_t name = (cubec_literal_identifier_t)gp->name;
  EXPECT_STREQ(string_get(name->value), "T");
  ASSERT_NE(gp->constraints, nullptr);
  EXPECT_EQ(vec_get_size(gp->constraints), 1);

  allocator_free(allocator, &params);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_generic_param, parse_multiple_params) {
  const char *source = "[A, B]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  vec_t params = read_generic_params(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(params, nullptr);
  EXPECT_EQ(vec_get_size(params), 2);

  allocator_free(allocator, &params);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_generic_param, non_bracket_returns_null) {
  const char *source = "T";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  vec_t params = read_generic_params(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(params, nullptr);

  allocator_free(allocator, &tokens);
}

TEST_F(dt_generic_param, write_simple) {
  const char *source = "[T]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  vec_t params = read_generic_params(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(params, nullptr);
  node_t param = (node_t)vec_get(params, 0);
  ASSERT_NE(param, nullptr);

  writer_t writer = (writer_t)allocator_create(allocator, &g_writer_type, NULL);
  write_generic_param(writer, param);
  string_t result = writer_get_string(writer); const char *output = string_get(result);
  EXPECT_STREQ(output, "T");

  allocator_free(allocator, &result); allocator_free(allocator, &writer);
  allocator_free(allocator, &params);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_generic_param, write_with_constraint) {
  const char *source = "[T extends Hashable]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  vec_t params = read_generic_params(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(params, nullptr);
  node_t param = (node_t)vec_get(params, 0);
  ASSERT_NE(param, nullptr);

  writer_t writer = (writer_t)allocator_create(allocator, &g_writer_type, NULL);
  write_generic_param(writer, param);
  string_t result = writer_get_string(writer); const char *output = string_get(result);
  EXPECT_STREQ(output, "T extends Hashable");

  allocator_free(allocator, &result); allocator_free(allocator, &writer);
  allocator_free(allocator, &params);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_generic_param, write_value_param) {
  const char *source = "[N: u64]";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  vec_t params = read_generic_params(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(params, nullptr);
  node_t param = (node_t)vec_get(params, 0);
  ASSERT_NE(param, nullptr);

  writer_t writer = (writer_t)allocator_create(allocator, &g_writer_type, NULL);
  write_generic_param(writer, param);
  string_t result = writer_get_string(writer); const char *output = string_get(result);
  EXPECT_STREQ(output, "N: u64");

  allocator_free(allocator, &result); allocator_free(allocator, &writer);
  allocator_free(allocator, &params);
  allocator_free(allocator, &tokens);
}
