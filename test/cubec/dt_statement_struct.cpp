#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/statement.h"
#include "cubec/statement_struct.h"
#include "cubec/struct_field.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/statement_function.h"
#include "cubec/generic_param.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_statement_struct : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ==========================================================================
 *  Basic parsing
 * ========================================================================== */

/* ---- Basic struct: struct Point { x: f64; y: f64; } ---- */

TEST_F(dt_statement_struct, basic_struct) {
  const char *source = "struct Point { x: f64; y: f64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_STRUCT);

  cubec_statement_struct_t struct_node = (cubec_statement_struct_t)node;
  EXPECT_FALSE(struct_node->is_export);
  ASSERT_NE(struct_node->name, nullptr);
  EXPECT_EQ(struct_node->name->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(struct_node->generic_params, nullptr);
  ASSERT_NE(struct_node->members, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->members), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Empty struct ---- */

TEST_F(dt_statement_struct, struct_empty) {
  const char *source = "struct Empty { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_STRUCT);

  cubec_statement_struct_t struct_node = (cubec_statement_struct_t)node;
  EXPECT_EQ(struct_node->generic_params, nullptr);
  ASSERT_NE(struct_node->members, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->members), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Struct with instance fields ---- */

TEST_F(dt_statement_struct, struct_with_instance_fields) {
  const char *source = "struct Point { x: f64; y: f64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_struct_t struct_node = (cubec_statement_struct_t)node;
  ASSERT_NE(struct_node->members, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->members), 2);

  /* First field */
  node_t field0 = (node_t)vec_get(struct_node->members, 0);
  EXPECT_EQ(field0->kind, CUBEC_NODE_STRUCT_FIELD);
  cubec_struct_field_t field = (cubec_struct_field_t)field0;
  EXPECT_FALSE(field->is_pub);
  ASSERT_NE(field->name, nullptr);
  ASSERT_NE(field->type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Struct with pub field ---- */

TEST_F(dt_statement_struct, struct_with_pub_field) {
  const char *source = "struct Foo { pub x: i32; y: i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_struct_t struct_node = (cubec_statement_struct_t)node;
  ASSERT_NE(struct_node->members, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->members), 2);

  /* First field is pub */
  node_t field0 = (node_t)vec_get(struct_node->members, 0);
  EXPECT_EQ(field0->kind, CUBEC_NODE_STRUCT_FIELD);
  cubec_struct_field_t field = (cubec_struct_field_t)field0;
  EXPECT_TRUE(field->is_pub);

  /* Second field is private */
  node_t field1 = (node_t)vec_get(struct_node->members, 1);
  field = (cubec_struct_field_t)field1;
  EXPECT_FALSE(field->is_pub);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Generic structs
 * ========================================================================== */

/* ---- Generic struct: struct Vec[T] { data: *T; len: u64; } ---- */

TEST_F(dt_statement_struct, struct_generic) {
  const char *source = "struct Vec[T] { data: *T; len: u64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_struct_t struct_node = (cubec_statement_struct_t)node;
  ASSERT_NE(struct_node->generic_params, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->generic_params), 1);
  ASSERT_NE(struct_node->members, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->members), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Multi-param generic: struct Map[K, V] ---- */

TEST_F(dt_statement_struct, struct_generic_multi) {
  const char *source = "struct Map[K, V] { var data: *K; var value: V; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_struct_t struct_node = (cubec_statement_struct_t)node;
  ASSERT_NE(struct_node->generic_params, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->generic_params), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Mixed members: fields, var, type, func
 * ========================================================================== */

/* ---- Struct with static var field ---- */

TEST_F(dt_statement_struct, struct_with_static_var) {
  const char *source = "struct Counter { var count: i32 = 0; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_struct_t struct_node = (cubec_statement_struct_t)node;
  ASSERT_NE(struct_node->members, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->members), 1);

  /* Static var is a statement_declaration (wraps declaration_variable) */
  node_t member = (node_t)vec_get(struct_node->members, 0);
  EXPECT_EQ(member->kind, CUBEC_NODE_STATEMENT_DECLARATION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Struct with associated type ---- */

TEST_F(dt_statement_struct, struct_with_type_member) {
  const char *source = "struct Foo { type Element = i32; var x: Element; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_struct_t struct_node = (cubec_statement_struct_t)node;
  ASSERT_NE(struct_node->members, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->members), 2);

  /* First member is type declaration */
  node_t member0 = (node_t)vec_get(struct_node->members, 0);
  EXPECT_EQ(member0->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Struct with method ---- */

TEST_F(dt_statement_struct, struct_with_method) {
  const char *source = "struct Foo { var x: i32; func bar(self): i32 { } }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_struct_t struct_node = (cubec_statement_struct_t)node;
  ASSERT_NE(struct_node->members, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->members), 2);

  /* Second member is function */
  node_t member1 = (node_t)vec_get(struct_node->members, 1);
  EXPECT_EQ(member1->kind, CUBEC_NODE_STATEMENT_FUNCTION);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Export modifier
 * ========================================================================== */

/* ---- Export struct ---- */

TEST_F(dt_statement_struct, export_struct) {
  const char *source = "export struct Point { x: f64; y: f64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_struct_t struct_node = (cubec_statement_struct_t)node;
  EXPECT_TRUE(struct_node->is_export);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Non-export struct ---- */

TEST_F(dt_statement_struct, non_export_struct) {
  const char *source = "struct Point { x: f64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_struct_t struct_node = (cubec_statement_struct_t)node;
  EXPECT_FALSE(struct_node->is_export);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Lifecycle: clone / move
 * ========================================================================== */

/* ---- Clone ---- */

TEST_F(dt_statement_struct, clone) {
  const char *source = "struct Foo { x: i32; func bar(self): i32 { } }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)alloc_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_STRUCT);

  cubec_statement_struct_t struct_node = (cubec_statement_struct_t)cloned;
  ASSERT_NE(struct_node->name, nullptr);
  ASSERT_NE(struct_node->members, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->members), 2);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Move ---- */

TEST_F(dt_statement_struct, move) {
  const char *source = "struct Foo { x: i32; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)alloc_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_STRUCT);

  cubec_statement_struct_t struct_node = (cubec_statement_struct_t)moved;
  ASSERT_NE(struct_node->name, nullptr);
  ASSERT_NE(struct_node->members, nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Token consumption
 * ========================================================================== */

TEST_F(dt_statement_struct, consume_all_tokens) {
  const char *source = "struct Point { x: f64; y: f64; }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(position, vec_get_size(tokens) - 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Integration
 * ========================================================================== */

/* ---- Via read_statement dispatcher ---- */

TEST_F(dt_statement_struct, via_read_statement) {
  const char *source = "struct Foo { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_STRUCT);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Via read_program_node ---- */

TEST_F(dt_statement_struct, via_read_program) {
  const char *source = "struct Foo { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_program_node(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_PROGRAM);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  implement clause parsing
 * ========================================================================== */

/* ---- struct with single implement ---- */

TEST_F(dt_statement_struct, implement_single) {
  const char *source = "struct Foo implement Printable { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_STRUCT);

  cubec_statement_struct_t struct_node = (cubec_statement_struct_t)node;
  ASSERT_NE(struct_node->implements, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->implements), 1u);
  EXPECT_EQ(struct_node->generic_params, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- struct with generic + implement ---- */

TEST_F(dt_statement_struct, implement_generic) {
  const char *source = "struct Vec[T] implement Iterable[T] { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_STRUCT);

  cubec_statement_struct_t struct_node = (cubec_statement_struct_t)node;
  ASSERT_NE(struct_node->implements, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->implements), 1u);
  ASSERT_NE(struct_node->generic_params, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- struct with multiple implement ---- */

TEST_F(dt_statement_struct, implement_multiple) {
  const char *source = "struct Foo implement A, B, C { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_STRUCT);

  cubec_statement_struct_t struct_node = (cubec_statement_struct_t)node;
  ASSERT_NE(struct_node->implements, nullptr);
  EXPECT_EQ(vec_get_size(struct_node->implements), 3u);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- struct without implement — implements is NULL ---- */

TEST_F(dt_statement_struct, no_implement) {
  const char *source = "struct Foo { }";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_struct(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_struct_t struct_node = (cubec_statement_struct_t)node;
  EXPECT_EQ(struct_node->implements, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_struct, write_empty_struct) {
  const char *source = "struct Empty { }";
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
  EXPECT_STREQ(output, "struct Empty {\n}\n");
  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
