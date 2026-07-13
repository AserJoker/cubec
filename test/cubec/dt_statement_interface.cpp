#include "cubec/statement.h"
#include "cubec/statement_interface.h"
#include "cubec/interface_method.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/function_argument.h"
#include "cubec/generic_param.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_statement_interface : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* ==========================================================================
 *  Basic parsing
 * ========================================================================== */

/* ---- Basic interface: interface Foo { } ---- */

TEST_F(dt_statement_interface, basic_interface) {
  const char *source = "interface Foo { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_interface(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_INTERFACE);

  cubec_statement_interface_t iface = (cubec_statement_interface_t)node;
  EXPECT_FALSE(iface->is_export);
  ASSERT_NE(iface->name, nullptr);
  EXPECT_EQ(iface->name->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_EQ(iface->generic_params, nullptr);
  ASSERT_NE(iface->members, nullptr);
  EXPECT_EQ(vec_get_size(iface->members), 0);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Interface with method: interface Iterator { func next(self): Item; } ---- */

TEST_F(dt_statement_interface, interface_with_method) {
  const char *source = "interface Iterator { func next(self): Item; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_interface(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_INTERFACE);

  cubec_statement_interface_t iface = (cubec_statement_interface_t)node;
  ASSERT_NE(iface->members, nullptr);
  EXPECT_EQ(vec_get_size(iface->members), 1);

  node_t member = (node_t)vec_get(iface->members, 0);
  EXPECT_EQ(member->kind, CUBEC_NODE_INTERFACE_METHOD);

  cubec_interface_method_t method = (cubec_interface_method_t)member;
  ASSERT_NE(method->name, nullptr);
  EXPECT_EQ(method->name->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  ASSERT_NE(method->arguments, nullptr);
  EXPECT_EQ(vec_get_size(method->arguments), 1);
  ASSERT_NE(method->return_type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Interface with type member and method ---- */

TEST_F(dt_statement_interface, interface_with_type_member) {
  const char *source = "interface Iterator { type Item; func next(self): Item; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_interface(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_interface_t iface = (cubec_statement_interface_t)node;
  ASSERT_NE(iface->members, nullptr);
  EXPECT_EQ(vec_get_size(iface->members), 2);

  /* First member: associated type */
  node_t member0 = (node_t)vec_get(iface->members, 0);
  EXPECT_EQ(member0->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);

  cubec_statement_declaration_type_t type_decl = (cubec_statement_declaration_type_t)member0;
  EXPECT_FALSE(type_decl->is_export);
  EXPECT_FALSE(type_decl->is_builtin);
  ASSERT_NE(type_decl->name, nullptr);
  EXPECT_EQ(type_decl->type_value, nullptr);

  /* Second member: method */
  node_t member1 = (node_t)vec_get(iface->members, 1);
  EXPECT_EQ(member1->kind, CUBEC_NODE_INTERFACE_METHOD);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Generic interfaces
 * ========================================================================== */

/* ---- Generic interface: interface Container[T] { ... } ---- */

TEST_F(dt_statement_interface, interface_generic) {
  const char *source = "interface Container[T] { func len(self): u64; func get(self, idx: u64): T; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_interface(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_interface_t iface = (cubec_statement_interface_t)node;
  ASSERT_NE(iface->generic_params, nullptr);
  EXPECT_EQ(vec_get_size(iface->generic_params), 1);
  ASSERT_NE(iface->members, nullptr);
  EXPECT_EQ(vec_get_size(iface->members), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Multi-param generic: interface Mapper[K, V] { ... } ---- */

TEST_F(dt_statement_interface, interface_generic_multi) {
  const char *source = "interface Mapper[K, V] { func map(self, key: K): V; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_interface(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_interface_t iface = (cubec_statement_interface_t)node;
  ASSERT_NE(iface->generic_params, nullptr);
  EXPECT_EQ(vec_get_size(iface->generic_params), 2);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Method details
 * ========================================================================== */

/* ---- Method with no return type (void) ---- */

TEST_F(dt_statement_interface, interface_method_no_return_type) {
  const char *source = "interface Foo { func bar(); }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_interface(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_interface_t iface = (cubec_statement_interface_t)node;
  ASSERT_NE(iface->members, nullptr);
  EXPECT_EQ(vec_get_size(iface->members), 1);

  cubec_interface_method_t method = (cubec_interface_method_t)vec_get(iface->members, 0);
  EXPECT_EQ(method->return_type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Method with generic params ---- */

TEST_F(dt_statement_interface, interface_method_generic) {
  const char *source = "interface Foo { func identity[T](x: T): T; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_interface(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_interface_t iface = (cubec_statement_interface_t)node;
  cubec_interface_method_t method = (cubec_interface_method_t)vec_get(iface->members, 0);
  ASSERT_NE(method->generic_params, nullptr);
  EXPECT_EQ(vec_get_size(method->generic_params), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Method with pointer type ---- */

TEST_F(dt_statement_interface, interface_method_with_pointer_type) {
  const char *source = "interface Foo { func read(self): *u8; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_interface(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_interface_t iface = (cubec_statement_interface_t)node;
  cubec_interface_method_t method = (cubec_interface_method_t)vec_get(iface->members, 0);
  ASSERT_NE(method->return_type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Method with slice type ---- */

TEST_F(dt_statement_interface, interface_method_with_slice_type) {
  const char *source = "interface Foo { func data(self): []u8; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_interface(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_interface_t iface = (cubec_statement_interface_t)node;
  cubec_interface_method_t method = (cubec_interface_method_t)vec_get(iface->members, 0);
  ASSERT_NE(method->return_type, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Modifiers
 * ========================================================================== */

/* ---- Export interface ---- */

TEST_F(dt_statement_interface, export_interface) {
  const char *source = "export interface Foo { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_interface(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_interface_t iface = (cubec_statement_interface_t)node;
  EXPECT_TRUE(iface->is_export);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Non-export interface ---- */

TEST_F(dt_statement_interface, non_export_interface) {
  const char *source = "interface Foo { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_interface(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_interface_t iface = (cubec_statement_interface_t)node;
  EXPECT_FALSE(iface->is_export);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Export interface with method ---- */

TEST_F(dt_statement_interface, export_interface_with_method) {
  const char *source = "export interface Serializable { func serialize(self): []u8; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_interface(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_interface_t iface = (cubec_statement_interface_t)node;
  EXPECT_TRUE(iface->is_export);
  ASSERT_NE(iface->members, nullptr);
  EXPECT_EQ(vec_get_size(iface->members), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Lifecycle: clone / move
 * ========================================================================== */

/* ---- Clone ---- */

TEST_F(dt_statement_interface, clone) {
  const char *source = "interface Foo { func bar(self): i32; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_interface(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_INTERFACE);

  cubec_statement_interface_t iface = (cubec_statement_interface_t)cloned;
  ASSERT_NE(iface->name, nullptr);
  ASSERT_NE(iface->members, nullptr);
  EXPECT_EQ(vec_get_size(iface->members), 1);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Move ---- */

TEST_F(dt_statement_interface, move) {
  const char *source = "interface Foo { func bar(self): i32; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_interface(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_INTERFACE);

  cubec_statement_interface_t iface = (cubec_statement_interface_t)moved;
  ASSERT_NE(iface->name, nullptr);
  ASSERT_NE(iface->members, nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Token consumption
 * ========================================================================== */

/* ---- Consume all tokens ---- */

TEST_F(dt_statement_interface, consume_all_tokens) {
  const char *source = "interface Foo { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_interface(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(position, vec_get_size(tokens) - 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Integration
 * ========================================================================== */

/* ---- Via read_statement dispatcher ---- */

TEST_F(dt_statement_interface, via_read_statement) {
  const char *source = "interface Foo { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_INTERFACE);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Via read_program_node ---- */

TEST_F(dt_statement_interface, via_read_program) {
  const char *source = "interface Foo { }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_program_node(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_PROGRAM);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ==========================================================================
 *  Associated type with generic params
 * ========================================================================== */

/* ---- Type member with generic: type Item[T]; ---- */

TEST_F(dt_statement_interface, interface_type_member_with_generic) {
  const char *source = "interface Foo { type Item[T]; func get(self): Item[T]; }";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_interface(allocator, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  cubec_statement_interface_t iface = (cubec_statement_interface_t)node;
  ASSERT_NE(iface->members, nullptr);
  EXPECT_EQ(vec_get_size(iface->members), 2);

  /* First member: type Item[T] */
  node_t member0 = (node_t)vec_get(iface->members, 0);
  EXPECT_EQ(member0->kind, CUBEC_NODE_STATEMENT_DECLARATION_TYPE);
  cubec_statement_declaration_type_t type_decl = (cubec_statement_declaration_type_t)member0;
  ASSERT_NE(type_decl->params, nullptr);
  EXPECT_EQ(vec_get_size(type_decl->params), 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
