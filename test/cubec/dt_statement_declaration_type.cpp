#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/statement.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/generic_param.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_statement_declaration_type : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ---- Simple type alias: type MyInt = i32; ---- */

TEST_F(dt_statement_declaration_type, simple_alias) {
  const char *source = "type MyInt = i32;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
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
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
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
  EXPECT_EQ(param->constraints, nullptr);
  EXPECT_EQ(param->value_type, nullptr);

  EXPECT_NE(decl->type_value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Type alias with multiple generic parameters: type Pair[A, B] = i32; ---- */

TEST_F(dt_statement_declaration_type, multiple_generic_params) {
  const char *source = "type Pair[A, B] = i32;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
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
    EXPECT_EQ(param->constraints, nullptr);
    EXPECT_EQ(param->value_type, nullptr);
  }

  EXPECT_NE(decl->type_value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Type alias with extends constraint: type NumericVec[T extends Numeric] = Vec[T]; ---- */

TEST_F(dt_statement_declaration_type, generic_param_with_constraint) {
  const char *source = "type NumericVec[T extends Numeric] = Vec[T];";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_NE(decl->params, nullptr);
  EXPECT_EQ(vec_get_size(decl->params), 1);

  cubec_generic_param_t param = (cubec_generic_param_t)vec_get(decl->params, 0);
  EXPECT_EQ(param->super.kind, CUBEC_NODE_GENERIC_PARAM);
  EXPECT_NE(param->name, nullptr);
  EXPECT_EQ(param->name->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_NE(param->constraints, nullptr);
  EXPECT_EQ(param->value_type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Type alias with value generic: type FixedArray[N: u64, T] = [N]T; ---- */

TEST_F(dt_statement_declaration_type, generic_param_with_value_type) {
  const char *source = "type FixedArray[N: u64, T] = [N]T;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_NE(decl->params, nullptr);
  EXPECT_EQ(vec_get_size(decl->params), 2);

  /* First param: N: u64 (value generic) */
  cubec_generic_param_t param0 = (cubec_generic_param_t)vec_get(decl->params, 0);
  EXPECT_EQ(param0->super.kind, CUBEC_NODE_GENERIC_PARAM);
  EXPECT_NE(param0->name, nullptr);
  EXPECT_EQ(param0->constraints, nullptr);
  EXPECT_NE(param0->value_type, nullptr);

  /* Second param: T (simple type generic) */
  cubec_generic_param_t param1 = (cubec_generic_param_t)vec_get(decl->params, 1);
  EXPECT_EQ(param1->super.kind, CUBEC_NODE_GENERIC_PARAM);
  EXPECT_NE(param1->name, nullptr);
  EXPECT_EQ(param1->constraints, nullptr);
  EXPECT_EQ(param1->value_type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Complex type expression: type MyPtr = *i32; ---- */

TEST_F(dt_statement_declaration_type, pointer_type_expression) {
  const char *source = "type MyPtr = *i32;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
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
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* type, MyInt, =, i32, ;, + whitespace/comment tokens + EOF */
  EXPECT_EQ(position, vec_get_size(tokens) - 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Clone ---- */

TEST_F(dt_statement_declaration_type, clone) {
  const char *source = "type MyInt = i32;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
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
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
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
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)alloc_move(allocator, node);
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
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
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
  EXPECT_EQ(param->constraints, nullptr);
  EXPECT_EQ(param->value_type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Rest param with regular params: type Tuple[T, ...Rest] = i32; ---- */

TEST_F(dt_statement_declaration_type, rest_param_after_regular) {
  const char *source = "type Tuple[T, ...Rest] = i32;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
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
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_NE(decl->params, nullptr);
  EXPECT_EQ(vec_get_size(decl->params), 2);

  /* Second param: ...Rest extends Numeric */
  cubec_generic_param_t param1 = (cubec_generic_param_t)vec_get(decl->params, 1);
  EXPECT_EQ(param1->super.kind, CUBEC_NODE_GENERIC_PARAM);
  EXPECT_TRUE(param1->is_rest);
  EXPECT_NE(param1->constraints, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Clone with rest param ---- */

TEST_F(dt_statement_declaration_type, clone_with_rest_param) {
  const char *source = "type Variadic[...Args] = i32;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
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
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  cubec_generic_param_t param = (cubec_generic_param_t)vec_get(decl->params, 0);
  EXPECT_FALSE(param->is_rest);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Export simple type alias ---- */

TEST_F(dt_statement_declaration_type, export_simple_alias) {
  const char *source = "export type MyInt = i32;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_TRUE(decl->is_export);
  EXPECT_NE(decl->name, nullptr);
  EXPECT_EQ(decl->params, nullptr);
  EXPECT_NE(decl->type_value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Export generic type alias ---- */

TEST_F(dt_statement_declaration_type, export_generic_alias) {
  const char *source = "export type Vec3[T] = Vec[Vec[Vec[T]]];";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_TRUE(decl->is_export);
  EXPECT_NE(decl->params, nullptr);
  EXPECT_EQ(vec_get_size(decl->params), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Non-export type alias ---- */

TEST_F(dt_statement_declaration_type, non_export_alias) {
  const char *source = "type MyInt = i32;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_FALSE(decl->is_export);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Export with pointer type expression ---- */

TEST_F(dt_statement_declaration_type, export_pointer_type) {
  const char *source = "export type MyPtr = *i32;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_TRUE(decl->is_export);
  EXPECT_NE(decl->type_value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Export clone ---- */

TEST_F(dt_statement_declaration_type, export_clone) {
  const char *source = "export type MyInt = i32;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)cloned;
  EXPECT_TRUE(decl->is_export);
  EXPECT_NE(decl->name, nullptr);
  EXPECT_NE(decl->type_value, nullptr);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Export move ---- */

TEST_F(dt_statement_declaration_type, export_move) {
  const char *source = "export type Vec3[T] = Vec[Vec[Vec[T]]];";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)alloc_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)moved;
  EXPECT_TRUE(decl->is_export);
  EXPECT_NE(decl->params, nullptr);
  EXPECT_EQ(vec_get_size(decl->params), 1);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Builtin type
 * ========================================================================== */

/* ---- Builtin type: builtin type RemoveConst[T]; ---- */

TEST_F(dt_statement_declaration_type, builtin_type_no_body) {
  const char *source = "builtin type RemoveConst[T];";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_TRUE(decl->is_builtin);
  EXPECT_FALSE(decl->is_export);
  EXPECT_NE(decl->name, nullptr);
  EXPECT_NE(decl->params, nullptr);
  EXPECT_EQ(decl->type_value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Export builtin type (orthogonal) ---- */

TEST_F(dt_statement_declaration_type, export_builtin_type) {
  const char *source = "export builtin type Ptr[T];";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_TRUE(decl->is_export);
  EXPECT_TRUE(decl->is_builtin);
  EXPECT_EQ(decl->type_value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Builtin type without generic params ---- */

TEST_F(dt_statement_declaration_type, builtin_type_no_params) {
  const char *source = "builtin type Void;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  EXPECT_TRUE(decl->is_builtin);
  EXPECT_NE(decl->name, nullptr);
  EXPECT_EQ(decl->params, nullptr);
  EXPECT_EQ(decl->type_value, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Builtin type clone ---- */

TEST_F(dt_statement_declaration_type, builtin_type_clone) {
  const char *source = "builtin type RemoveConst[T];";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)cloned;
  EXPECT_TRUE(decl->is_builtin);
  EXPECT_EQ(decl->type_value, nullptr);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Builtin type move ---- */

TEST_F(dt_statement_declaration_type, builtin_type_move) {
  const char *source = "builtin type RemoveConst[T];";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)alloc_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)moved;
  EXPECT_TRUE(decl->is_builtin);
  EXPECT_EQ(decl->type_value, nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Multi-constraint with & (AND)
 * ========================================================================== */

/* ---- T extends A & B ---- */

TEST_F(dt_statement_declaration_type, multi_constraint_and) {
  const char *source = "type Foo[T extends Printable & Serializable] = T;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  ASSERT_NE(decl->params, nullptr);
  EXPECT_EQ(vec_get_size(decl->params), 1);

  cubec_generic_param_t param = (cubec_generic_param_t)vec_get(decl->params, 0);
  ASSERT_NE(param->constraints, nullptr);
  EXPECT_EQ(vec_get_size(param->constraints), 2u);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- T extends A & B & C ---- */

TEST_F(dt_statement_declaration_type, multi_constraint_three) {
  const char *source = "type Foo[T extends A & B & C] = T;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  cubec_generic_param_t param = (cubec_generic_param_t)vec_get(decl->params, 0);
  ASSERT_NE(param->constraints, nullptr);
  EXPECT_EQ(vec_get_size(param->constraints), 3u);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_declaration_type, write_type_alias) {
  const char *source = "type MyInt = i32;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_statement(ectx, node);
  emit_newline(ectx);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "type MyInt = i32;\n");
  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
