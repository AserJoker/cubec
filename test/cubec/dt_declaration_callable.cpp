#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/expression.h"
#include "cubec/declaration_pointer.h"
#include "cubec/declaration_slice.h"
#include "cubec/expression_subscript.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/declaration_qualifier.h"
#include "cubec/declaration_callable.h"
#include "cubec/expression_group.h"
#include "cubec/expression_ternary.h"
/* volatile now uses declaration_qualifier */
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include "core/vec.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_declaration_callable : public CubecTest {
protected:
};

/* --------------------------------------------------------------------------
 *  Basic function type expressions
 * -------------------------------------------------------------------------- */

/* Simple: func(i32) -> i32 */
TEST_F(dt_declaration_callable, simple) {
  const char *source = "func(i32) -> i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  cubec_declaration_callable_t func_node =
      (cubec_declaration_callable_t)node;
  ASSERT_NE(func_node->parameters, nullptr);
  EXPECT_EQ(vec_get_size(func_node->parameters), 1);
  EXPECT_NE(func_node->return_type, nullptr);
  EXPECT_EQ(func_node->return_type->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_FALSE(func_node->is_c_variadic);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Two params: func(i32, i32) -> void */
TEST_F(dt_declaration_callable, two_params) {
  const char *source = "func(i32, i32) -> void";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  cubec_declaration_callable_t func_node =
      (cubec_declaration_callable_t)node;
  EXPECT_EQ(vec_get_size(func_node->parameters), 2);
  EXPECT_NE(func_node->return_type, nullptr);
  EXPECT_FALSE(func_node->is_c_variadic);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* No params: func() -> i32 */
TEST_F(dt_declaration_callable, no_params) {
  const char *source = "func() -> i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  cubec_declaration_callable_t func_node =
      (cubec_declaration_callable_t)node;
  EXPECT_EQ(vec_get_size(func_node->parameters), 0);
  EXPECT_NE(func_node->return_type, nullptr);
  EXPECT_FALSE(func_node->is_c_variadic);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Complex parameter and return types
 * -------------------------------------------------------------------------- */

/* Pointer param and return: func(*i32) -> *i32 */
TEST_F(dt_declaration_callable, pointer_param_and_return) {
  const char *source = "func(*i32) -> *i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  cubec_declaration_callable_t func_node =
      (cubec_declaration_callable_t)node;
  EXPECT_EQ(vec_get_size(func_node->parameters), 1);
  /* First param is a pointer type */
  node_t param = (node_t)vec_get(func_node->parameters, 0);
  EXPECT_EQ(param->kind, CUBEC_NODE_DECLARATION_POINTER);
  /* Return type is pointer */
  EXPECT_EQ(func_node->return_type->kind, CUBEC_NODE_DECLARATION_POINTER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Generic type param: func(Vec[i32]) -> []i32 */
TEST_F(dt_declaration_callable, generic_param) {
  const char *source = "func(Vec[i32]) -> []i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  cubec_declaration_callable_t func_node =
      (cubec_declaration_callable_t)node;
  node_t param = (node_t)vec_get(func_node->parameters, 0);
  EXPECT_EQ(param->kind, CUBEC_NODE_EXPRESSION_SUBSCRIPT);
  EXPECT_EQ(func_node->return_type->kind, CUBEC_NODE_DECLARATION_SLICE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Namespace type param: func(std::vec::Vec) -> i32 */
TEST_F(dt_declaration_callable, namespace_param) {
  const char *source = "func(std::vec::Vec) -> i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  cubec_declaration_callable_t func_node =
      (cubec_declaration_callable_t)node;
  node_t param = (node_t)vec_get(func_node->parameters, 0);
  EXPECT_EQ(param->kind, CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  C-style variadic
 * -------------------------------------------------------------------------- */

/* func(i32, ...) -> void */
TEST_F(dt_declaration_callable, c_variadic) {
  const char *source = "func(i32, ...) -> void";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  cubec_declaration_callable_t func_node =
      (cubec_declaration_callable_t)node;
  EXPECT_EQ(vec_get_size(func_node->parameters), 1);
  EXPECT_TRUE(func_node->is_c_variadic);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* func(...) -> void 鈥?variadic only, no named params */
TEST_F(dt_declaration_callable, c_variadic_only) {
  const char *source = "func(...) -> void";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  cubec_declaration_callable_t func_node =
      (cubec_declaration_callable_t)node;
  EXPECT_EQ(vec_get_size(func_node->parameters), 0);
  EXPECT_TRUE(func_node->is_c_variadic);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Nesting: function type as parameter / wrapped by pointer/slice/const
 * -------------------------------------------------------------------------- */

/* Function type as parameter: func(func(i32) -> i32) -> void */
TEST_F(dt_declaration_callable, function_type_as_param) {
  const char *source = "func(func(i32) -> i32) -> void";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  cubec_declaration_callable_t outer =
      (cubec_declaration_callable_t)node;
  EXPECT_EQ(vec_get_size(outer->parameters), 1);

  /* The parameter is itself a function type */
  node_t param = (node_t)vec_get(outer->parameters, 0);
  EXPECT_EQ(param->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  cubec_declaration_callable_t inner =
      (cubec_declaration_callable_t)param;
  EXPECT_EQ(vec_get_size(inner->parameters), 1);
  EXPECT_NE(inner->return_type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Pointer to function type: *func(i32) -> i32 */
TEST_F(dt_declaration_callable, pointer_to_function) {
  const char *source = "*func(i32) -> i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_POINTER);

  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  ASSERT_NE(ptr->type, nullptr);
  EXPECT_EQ(ptr->type->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Slice of function type: []func(i32) -> i32 */
TEST_F(dt_declaration_callable, slice_of_function) {
  const char *source = "[]func(i32) -> i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_SLICE);

  cubec_declaration_slice_t slice = (cubec_declaration_slice_t)node;
  ASSERT_NE(slice->type, nullptr);
  EXPECT_EQ(slice->type->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Const function type: const func(i32) -> i32 */
TEST_F(dt_declaration_callable, const_function) {
  const char *source = "const func(i32) -> i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_QUALIFIER);

  cubec_declaration_qualifier_t const_node =
      (cubec_declaration_qualifier_t)node;
  ASSERT_NE(const_node->type, nullptr);
  EXPECT_EQ(const_node->type->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* Volatile function type: volatile func(i32) -> i32 */
TEST_F(dt_declaration_callable, volatile_function) {
  const char *source = "volatile func(i32) -> i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_QUALIFIER);

  cubec_declaration_qualifier_t vol_node =
      (cubec_declaration_qualifier_t)node;
  ASSERT_NE(vol_node->type, nullptr);
  EXPECT_EQ(vol_node->type->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Whitespace and token consumption
 * -------------------------------------------------------------------------- */

/* Consume all tokens */
TEST_F(dt_declaration_callable, consume_all_tokens) {
  const char *source = "func(i32) -> i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  /* All non-EOF tokens should be consumed */
  skip_whitespace(tokens, &position);
  token_t remaining = (token_t)vec_get(tokens, position);
  ASSERT_NE(remaining, nullptr);
  EXPECT_EQ(token_get_kind(remaining), CUBEC_TOKEN_EOF);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* With spaces around delimiters */
TEST_F(dt_declaration_callable, with_spaces) {
  const char *source = "func ( i32 , i32 ) -> i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  cubec_declaration_callable_t func_node =
      (cubec_declaration_callable_t)node;
  EXPECT_EQ(vec_get_size(func_node->parameters), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Fallback: non-function-type returns NULL
 * -------------------------------------------------------------------------- */

TEST_F(dt_declaration_callable, non_func_returns_null) {
  const char *source = "i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_declaration_callable(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Error cases
 * -------------------------------------------------------------------------- */

/* Missing open paren: func i32) -> i32 鈥?falls back to declaration_function which also fails */
TEST_F(dt_declaration_callable, missing_open_paren_returns_null) {
  const char *source = "func i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  /* read_declaration_callable should return NULL because no '(' follows func */
  node_t node = read_declaration_callable(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* Missing ')' is error */
TEST_F(dt_declaration_callable, missing_close_paren_error) {
  const char *source = "func(i32 -> i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_declaration_callable(ctx, tokens, &position, "test.cubec");
  /* Should fail 鈥?no ')' before '->' */
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* Missing '->' is error */
TEST_F(dt_declaration_callable, missing_arrow_error) {
  const char *source = "func(i32) i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_declaration_callable(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* Missing return type after '->' is error */
TEST_F(dt_declaration_callable, missing_return_type_error) {
  const char *source = "func(i32) ->";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_declaration_callable(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* Trailing comma is error */
TEST_F(dt_declaration_callable, trailing_comma_error) {
  const char *source = "func(i32,) -> i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_declaration_callable(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));
  allocator_free(allocator, &node);

  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Greedy ternary consumption
 * -------------------------------------------------------------------------- */

/* func(i32) -> A ? B : C 鈫?func(i32) -> ternary(A, B, C) */
TEST_F(dt_declaration_callable, greedy_ternary_return) {
  const char *source = "func(i32) -> A ? B : C";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  cubec_declaration_callable_t func_node =
      (cubec_declaration_callable_t)node;
  ASSERT_NE(func_node->return_type, nullptr);
  EXPECT_EQ(func_node->return_type->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  cubec_expression_ternary_t ternary =
      (cubec_expression_ternary_t)func_node->return_type;
  ASSERT_NE(ternary->condition, nullptr);
  EXPECT_EQ(ternary->condition->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->consequent, nullptr);
  EXPECT_EQ(ternary->consequent->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(ternary->alternate, nullptr);
  EXPECT_EQ(ternary->alternate->kind, CUBEC_NODE_LITERAL_IDENTIFIER);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* func(A ? B : C) -> i32 鈫?ternary as parameter type */
TEST_F(dt_declaration_callable, greedy_ternary_param) {
  const char *source = "func(A ? B : C) -> i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  cubec_declaration_callable_t func_node =
      (cubec_declaration_callable_t)node;
  EXPECT_EQ(vec_get_size(func_node->parameters), 1);
  node_t param = (node_t)vec_get(func_node->parameters, 0);
  EXPECT_EQ(param->kind, CUBEC_NODE_EXPRESSION_TERNARY);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* --------------------------------------------------------------------------
 *  Clone and move
 * -------------------------------------------------------------------------- */

TEST_F(dt_declaration_callable, clone) {
  const char *source = "func(i32, i32) -> void";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  cubec_declaration_callable_t orig =
      (cubec_declaration_callable_t)node;
  cubec_declaration_callable_t copy =
      (cubec_declaration_callable_t)cloned;
  EXPECT_NE(orig->parameters, copy->parameters);
  EXPECT_EQ(vec_get_size(copy->parameters), 2);
  EXPECT_NE(orig->return_type, copy->return_type);

  allocator_free(allocator, &node);
  allocator_free(allocator, &cloned);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_declaration_callable, move) {
  const char *source = "func(i32) -> i32";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_expression_type(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)alloc_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_DECLARATION_CALLABLE);

  /* alloc_move transfers data but does NOT free the source pointer. */
  allocator_free(allocator, &node);

  cubec_declaration_callable_t result =
      (cubec_declaration_callable_t)moved;
  EXPECT_EQ(vec_get_size(result->parameters), 1);
  EXPECT_NE(result->return_type, nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_declaration_callable, write_callable) {
  const char *source = "func(i32) -> i32";
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
  EXPECT_STREQ(output, "func(i32) -> i32");
  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
