#include "ast/enum_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "common/cubec_test.hpp"
#include "core/allocator.h"
#include "core/position.h"
#include "reader/token.h"
#include <cstring>
#include <gtest/gtest.h>
class test_enum : public cubec_test {};
TEST_F(test_enum, normal) {
  position_t pos = {
      .offset = R"(pub enum Color : i32 { Red, Green = 2 })",
      .column = 0,
      .line = 0,
  };
  size_t len = strlen(pos.offset);
  token_stream_t stream =
      create_token_stream(allocator, &pos, pos.offset + len, "./test.cubec");
  ASSERT_NE(stream, nullptr);
  ast_node_t node = read_enum_declarator(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_ENUM_DECLARATOR);
  ast_node_t identifier = ast_get_child(node, "identifier");
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t pub = ast_get_child(node, "accessor");
  ast_node_t options = ast_get_child(node, "options");
  ASSERT_TRUE(node_location_is(pub, stream, "pub"));
  ASSERT_NE(identifier, nullptr);
  ASSERT_TRUE(node_location_is(identifier, stream, "Color"));
  ASSERT_NE(type, nullptr);
  ASSERT_TRUE(node_location_is(type, stream, "i32"));
  ASSERT_EQ(ast_get_length(options), 2);
  ast_node_t option = ast_get_item(options, 0);
  ASSERT_NE(option, nullptr);
  ast_node_t name = ast_get_child(option, "identifier");
  ast_node_t value = ast_get_child(option, "value");
  ASSERT_TRUE(node_location_is(name, stream, "Red"));
  ASSERT_EQ(value, nullptr);
  option = ast_get_item(options, 1);
  ASSERT_NE(option, nullptr);
  name = ast_get_child(option, "identifier");
  value = ast_get_child(option, "value");
  ASSERT_TRUE(node_location_is(name, stream, "Green"));
  ASSERT_TRUE(node_location_is(value, stream, "2"));
  allocator_free(allocator, node);
  allocator_free(allocator, stream);
}

TEST_F(test_enum, no_type) {
  position_t pos = {
      .offset = R"(pub enum Color  { Red, Green = 2 })",
      .column = 0,
      .line = 0,
  };
  size_t len = strlen(pos.offset);
  token_stream_t stream =
      create_token_stream(allocator, &pos, pos.offset + len, "./test.cubec");
  ASSERT_NE(stream, nullptr);
  ast_node_t node = read_enum_declarator(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_ENUM_DECLARATOR);
  ast_node_t identifier = ast_get_child(node, "identifier");
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t pub = ast_get_child(node, "accessor");
  ast_node_t options = ast_get_child(node, "options");
  ASSERT_TRUE(node_location_is(pub, stream, "pub"));
  ASSERT_NE(identifier, nullptr);
  ASSERT_TRUE(node_location_is(identifier, stream, "Color"));
  ASSERT_EQ(type, nullptr);
  ASSERT_EQ(ast_get_length(options), 2);
  ast_node_t option = ast_get_item(options, 0);
  ASSERT_NE(option, nullptr);
  ast_node_t name = ast_get_child(option, "identifier");
  ast_node_t value = ast_get_child(option, "value");
  ASSERT_TRUE(node_location_is(name, stream, "Red"));
  ASSERT_EQ(value, nullptr);
  option = ast_get_item(options, 1);
  ASSERT_NE(option, nullptr);
  name = ast_get_child(option, "identifier");
  value = ast_get_child(option, "value");
  ASSERT_TRUE(node_location_is(name, stream, "Green"));
  ASSERT_TRUE(node_location_is(value, stream, "2"));
  allocator_free(allocator, node);
  allocator_free(allocator, stream);
}

TEST_F(test_enum, no_identifier) {
  position_t pos = {
      .offset = R"(pub enum :i32  { Red, Green = 2 })",
      .column = 0,
      .line = 0,
  };
  size_t len = strlen(pos.offset);
  token_stream_t stream =
      create_token_stream(allocator, &pos, pos.offset + len, "./test.cubec");
  ASSERT_NE(stream, nullptr);
  ast_node_t node = read_enum_declarator(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_ENUM_DECLARATOR);
  ast_node_t identifier = ast_get_child(node, "identifier");
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t pub = ast_get_child(node, "accessor");
  ast_node_t options = ast_get_child(node, "options");
  ASSERT_TRUE(node_location_is(pub, stream, "pub"));
  ASSERT_EQ(identifier, nullptr);
  ASSERT_NE(type, nullptr);
  ASSERT_TRUE(node_location_is(type, stream, "i32"));
  ASSERT_EQ(ast_get_length(options), 2);
  ast_node_t option = ast_get_item(options, 0);
  ASSERT_NE(option, nullptr);
  ast_node_t name = ast_get_child(option, "identifier");
  ast_node_t value = ast_get_child(option, "value");
  ASSERT_TRUE(node_location_is(name, stream, "Red"));
  ASSERT_EQ(value, nullptr);
  option = ast_get_item(options, 1);
  ASSERT_NE(option, nullptr);
  name = ast_get_child(option, "identifier");
  value = ast_get_child(option, "value");
  ASSERT_TRUE(node_location_is(name, stream, "Green"));
  ASSERT_TRUE(node_location_is(value, stream, "2"));
  allocator_free(allocator, node);
  allocator_free(allocator, stream);
}