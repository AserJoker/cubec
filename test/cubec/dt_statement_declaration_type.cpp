#include "cubec/statement.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/generic_param.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_statement_declaration_type : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* ---- Simple type alias: type MyInt = i32; ---- */

TEST_F(dt_statement_declaration_type, simple_alias) {
  const char *source = "type MyInt = i32;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_NE(decl->name, nullptr);
  EXPECT_EQ(decl->name->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(decl->params, nullptr);
  EXPECT_NE(decl->type_value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Type alias with single generic parameter: type Vec3[T] = Vec[Vec[Vec[T]]]; ---- */

TEST_F(dt_statement_declaration_type, single_generic_param) {
  const char *source = "type Vec3[T] = Vec[Vec[Vec[T]]];";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_NE(decl->name, nullptr);
  EXPECT_EQ(decl->name->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_NE(decl->params, nullptr);
  EXPECT_EQ(vec_get_size(decl->params), 1);

  /* Verify the param is a generic_param node with name = "T" */
  cubec_generic_param_t param = (cubec_generic_param_t)vec_get(decl->params, 0);
  EXPECT_EQ(param->super.kind, CUBEC_NODE_GENERIC_PARAM);
  EXPECT_NE(param->name, nullptr);
  EXPECT_EQ(param->name->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(param->constraint, nullptr);
  EXPECT_EQ(param->value_type, nullptr);

  EXPECT_NE(decl->type_value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Type alias with multiple generic parameters: type Pair[A, B] = i32; ---- */

TEST_F(dt_statement_declaration_type, multiple_generic_params) {
  const char *source = "type Pair[A, B] = i32;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_NE(decl->name, nullptr);
  EXPECT_NE(decl->params, nullptr);
  EXPECT_EQ(vec_get_size(decl->params), 2);

  /* Verify both params are generic_param nodes */
  for (size_t i = 0; i < 2; i++) {
    cubec_generic_param_t param = (cubec_generic_param_t)vec_get(decl->params, i);
    EXPECT_EQ(param->super.kind, CUBEC_NODE_GENERIC_PARAM);
    EXPECT_NE(param->name, nullptr);
    EXPECT_EQ(param->name->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
    EXPECT_EQ(param->constraint, nullptr);
    EXPECT_EQ(param->value_type, nullptr);
  }

  EXPECT_NE(decl->type_value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Type alias with extends constraint: type NumericVec[T extends Numeric] = Vec[T]; ---- */

TEST_F(dt_statement_declaration_type, generic_param_with_constraint) {
  const char *source = "type NumericVec[T extends Numeric] = Vec[T];";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_NE(decl->params, nullptr);
  EXPECT_EQ(vec_get_size(decl->params), 1);

  cubec_generic_param_t param = (cubec_generic_param_t)vec_get(decl->params, 0);
  EXPECT_EQ(param->super.kind, CUBEC_NODE_GENERIC_PARAM);
  EXPECT_NE(param->name, nullptr);
  EXPECT_EQ(param->name->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_NE(param->constraint, nullptr);
  EXPECT_EQ(param->value_type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Type alias with value generic: type FixedArray[N: u64, T] = [N]T; ---- */

TEST_F(dt_statement_declaration_type, generic_param_with_value_type) {
  const char *source = "type FixedArray[N: u64, T] = [N]T;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_NE(decl->params, nullptr);
  EXPECT_EQ(vec_get_size(decl->params), 2);

  /* First param: N: u64 (value generic) */
  cubec_generic_param_t param0 = (cubec_generic_param_t)vec_get(decl->params, 0);
  EXPECT_EQ(param0->super.kind, CUBEC_NODE_GENERIC_PARAM);
  EXPECT_NE(param0->name, nullptr);
  EXPECT_EQ(param0->constraint, nullptr);
  EXPECT_NE(param0->value_type, nullptr);

  /* Second param: T (simple type generic) */
  cubec_generic_param_t param1 = (cubec_generic_param_t)vec_get(decl->params, 1);
  EXPECT_EQ(param1->super.kind, CUBEC_NODE_GENERIC_PARAM);
  EXPECT_NE(param1->name, nullptr);
  EXPECT_EQ(param1->constraint, nullptr);
  EXPECT_EQ(param1->value_type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Complex type expression: type MyPtr = *i32; ---- */

TEST_F(dt_statement_declaration_type, pointer_type_expression) {
  const char *source = "type MyPtr = *i32;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_NE(decl->type_value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Consume all tokens ---- */

TEST_F(dt_statement_declaration_type, consume_all_tokens) {
  const char *source = "type MyInt = i32;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* type, MyInt, =, i32, ;, + whitespace/comment tokens + EOF */
  EXPECT_EQ(position, vec_get_size(tokens) - 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Clone ---- */

TEST_F(dt_statement_declaration_type, clone) {
  const char *source = "type MyInt = i32;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)cloned;
  EXPECT_NE(decl->name, nullptr);
  EXPECT_NE(decl->type_value, nullptr);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Clone with generic params ---- */

TEST_F(dt_statement_declaration_type, clone_with_generic_params) {
  const char *source = "type Vec3[T] = Vec[Vec[Vec[T]]];";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)cloned;
  EXPECT_NE(decl->params, nullptr);
  EXPECT_EQ(vec_get_size(decl->params), 1);

  cubec_generic_param_t param = (cubec_generic_param_t)vec_get(decl->params, 0);
  EXPECT_EQ(param->super.kind, CUBEC_NODE_GENERIC_PARAM);
  EXPECT_NE(param->name, nullptr);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Move ---- */

TEST_F(dt_statement_declaration_type, move) {
  const char *source = "type Vec3[T] = Vec[Vec[Vec[T]]];";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)moved;
  EXPECT_NE(decl->name, nullptr);
  EXPECT_NE(decl->params, nullptr);
  EXPECT_EQ(vec_get_size(decl->params), 1);

  cubec_generic_param_t param = (cubec_generic_param_t)vec_get(decl->params, 0);
  EXPECT_EQ(param->super.kind, CUBEC_NODE_GENERIC_PARAM);
  EXPECT_NE(param->name, nullptr);

  EXPECT_NE(decl->type_value, nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Rest param (single): type Variadic[...Args] = i32; ---- */

TEST_F(dt_statement_declaration_type, rest_param_single) {
  const char *source = "type Variadic[...Args] = i32;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_NE(decl->params, nullptr);
  EXPECT_EQ(vec_get_size(decl->params), 1);

  cubec_generic_param_t param = (cubec_generic_param_t)vec_get(decl->params, 0);
  EXPECT_EQ(param->super.kind, CUBEC_NODE_GENERIC_PARAM);
  EXPECT_NE(param->name, nullptr);
  EXPECT_EQ(param->name->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_TRUE(param->is_rest);
  EXPECT_EQ(param->constraint, nullptr);
  EXPECT_EQ(param->value_type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Rest param with regular params: type Tuple[T, ...Rest] = i32; ---- */

TEST_F(dt_statement_declaration_type, rest_param_after_regular) {
  const char *source = "type Tuple[T, ...Rest] = i32;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_NE(decl->params, nullptr);
  EXPECT_EQ(vec_get_size(decl->params), 2);

  /* First param: T (regular) */
  cubec_generic_param_t param0 = (cubec_generic_param_t)vec_get(decl->params, 0);
  EXPECT_EQ(param0->super.kind, CUBEC_NODE_GENERIC_PARAM);
  EXPECT_FALSE(param0->is_rest);

  /* Second param: ...Rest (rest) */
  cubec_generic_param_t param1 = (cubec_generic_param_t)vec_get(decl->params, 1);
  EXPECT_EQ(param1->super.kind, CUBEC_NODE_GENERIC_PARAM);
  EXPECT_TRUE(param1->is_rest);
  EXPECT_NE(param1->name, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Rest param with constraint: type Filter[T, ...Rest extends Numeric] = i32; ---- */

TEST_F(dt_statement_declaration_type, rest_param_with_constraint) {
  const char *source = "type Filter[T, ...Rest extends Numeric] = i32;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_NE(decl->params, nullptr);
  EXPECT_EQ(vec_get_size(decl->params), 2);

  /* Second param: ...Rest extends Numeric */
  cubec_generic_param_t param1 = (cubec_generic_param_t)vec_get(decl->params, 1);
  EXPECT_EQ(param1->super.kind, CUBEC_NODE_GENERIC_PARAM);
  EXPECT_TRUE(param1->is_rest);
  EXPECT_NE(param1->constraint, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Clone with rest param ---- */

TEST_F(dt_statement_declaration_type, clone_with_rest_param) {
  const char *source = "type Variadic[...Args] = i32;";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)cloned;
  EXPECT_NE(decl->params, nullptr);
  EXPECT_EQ(vec_get_size(decl->params), 1);

  cubec_generic_param_t param = (cubec_generic_param_t)vec_get(decl->params, 0);
  EXPECT_TRUE(param->is_rest);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Regular param is NOT rest ---- */

TEST_F(dt_statement_declaration_type, regular_param_is_not_rest) {
  const char *source = "type Vec3[T] = Vec[Vec[Vec[T]]];";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  cubec_generic_param_t param = (cubec_generic_param_t)vec_get(decl->params, 0);
  EXPECT_FALSE(param->is_rest);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
