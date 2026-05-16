#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "common/cubec_test.hpp"
#include "core/allocator.h"
#include "core/position.h"
#include "reader/token.h"
#include <cstring>
#include <gtest/gtest.h>
class test_expression : public cubec_test {};
TEST_F(test_expression, identifier) {
  position_t position = {
      .offset = "abc",
      .column = 0,
      .line = 0,
  };
  size_t len = strlen(position.offset);
  token_stream_t stream = create_token_stream(
      allocator, &position, position.offset + len, "./test.cubec");
  ASSERT_NE(stream, nullptr);
  ast_node_t node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(node, stream, "abc"));
  allocator_free(allocator, node);
  allocator_free(allocator, stream);
}

TEST_F(test_expression, member) {
  position_t position = {
      .offset = "abc.def",
      .column = 0,
      .line = 0,
  };
  size_t len = strlen(position.offset);
  token_stream_t stream = create_token_stream(
      allocator, &position, position.offset + len, "./test.cubec");
  ASSERT_NE(stream, nullptr);
  ast_node_t node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_EXPRESSION_MEMBER);
  ast_node_t host = ast_get_child(node, "host");
  ASSERT_EQ(host->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(host, stream, "abc"));
  ast_node_t field = ast_get_child(node, "field");
  ASSERT_TRUE(node_location_is(field, stream, "def"));
  allocator_free(allocator, node);
  allocator_free(allocator, stream);

  position = {
      .offset = "abc\n.\ndef",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_EXPRESSION_MEMBER);
  host = ast_get_child(node, "host");
  ASSERT_EQ(host->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(host, stream, "abc"));
  field = ast_get_child(node, "field");
  ASSERT_TRUE(node_location_is(field, stream, "def"));
  allocator_free(allocator, node);
  allocator_free(allocator, stream);
  position = {
      .offset = "abc //host\n. // field\ndef",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_EXPRESSION_MEMBER);
  host = ast_get_child(node, "host");
  ASSERT_EQ(host->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(host, stream, "abc"));
  field = ast_get_child(node, "field");
  ASSERT_TRUE(node_location_is(field, stream, "def"));
  allocator_free(allocator, node);
  allocator_free(allocator, stream);

  position = {
      .offset = "abc //host\n.//def",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_ERROR);
  allocator_free(allocator, node);
  allocator_free(allocator, stream);
}
TEST_F(test_expression, generics) {
  position_t position = {
      .offset = "abc [ a , b , c ]",
      .column = 0,
      .line = 0,
  };
  size_t len = strlen(position.offset);
  token_stream_t stream = create_token_stream(
      allocator, &position, position.offset + len, "./test.cubec");
  ASSERT_NE(stream, nullptr);
  ast_node_t node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_EXPRESSION_GENERICS);
  ast_node_t host = ast_get_child(node, "host");
  ASSERT_EQ(host->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(host, stream, "abc"));
  ast_node_t arguments = ast_get_child(node, "arguments");
  ASSERT_NE(arguments, nullptr);
  ASSERT_EQ(arguments->type, NODE_TYPE_LIST);
  ASSERT_EQ(ast_get_length(arguments), 3);
  ast_node_t argument = ast_get_item(arguments, 0);
  ASSERT_NE(argument, nullptr);
  ASSERT_EQ(argument->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(argument, stream, "a"));
  argument = ast_get_item(arguments, 1);
  ASSERT_NE(argument, nullptr);
  ASSERT_EQ(argument->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(argument, stream, "b"));
  argument = ast_get_item(arguments, 2);
  ASSERT_NE(argument, nullptr);
  ASSERT_EQ(argument->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(argument, stream, "c"));
  allocator_free(allocator, node);
  allocator_free(allocator, stream);

  position = {
      .offset = "abc []",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_EXPRESSION_GENERICS);
  host = ast_get_child(node, "host");
  ASSERT_EQ(host->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(host, stream, "abc"));
  arguments = ast_get_child(node, "arguments");
  ASSERT_NE(arguments, nullptr);
  ASSERT_EQ(arguments->type, NODE_TYPE_LIST);
  ASSERT_EQ(ast_get_length(arguments), 0);
  allocator_free(allocator, node);
  allocator_free(allocator, stream);

  position = {
      .offset = "abc [",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_ERROR);
  allocator_free(allocator, node);
  allocator_free(allocator, stream);

  position = {
      .offset = "abc [a,]",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_ERROR);
  allocator_free(allocator, node);
  allocator_free(allocator, stream);

  position = {
      .offset = "abc [,a]",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_ERROR);
  allocator_free(allocator, node);
  allocator_free(allocator, stream);
}

TEST_F(test_expression, slice) {
  position_t position = {
      .offset = "abc [ a : b ]",
      .column = 0,
      .line = 0,
  };
  size_t len = strlen(position.offset);
  token_stream_t stream = create_token_stream(
      allocator, &position, position.offset + len, "./test.cubec");
  ASSERT_NE(stream, nullptr);
  ast_node_t node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_EXPRESSION_SLICE);
  ast_node_t host = ast_get_child(node, "host");
  ASSERT_EQ(host->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(host, stream, "abc"));
  ast_node_t start = ast_get_child(node, "start");
  ASSERT_TRUE(node_location_is(start, stream, "a"));
  ast_node_t end = ast_get_child(node, "end");
  ASSERT_TRUE(node_location_is(end, stream, "b"));
  allocator_free(allocator, node);
  allocator_free(allocator, stream);

  position = {
      .offset = "abc[:b]",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_EXPRESSION_SLICE);
  host = ast_get_child(node, "host");
  ASSERT_EQ(host->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(host, stream, "abc"));
  start = ast_get_child(node, "start");
  ASSERT_EQ(start, nullptr);
  end = ast_get_child(node, "end");
  ASSERT_TRUE(node_location_is(end, stream, "b"));
  allocator_free(allocator, node);
  allocator_free(allocator, stream);

  position = {
      .offset = "abc[a:]",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_EXPRESSION_SLICE);
  host = ast_get_child(node, "host");
  ASSERT_EQ(host->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(host, stream, "abc"));
  start = ast_get_child(node, "start");
  ASSERT_TRUE(node_location_is(start, stream, "a"));
  end = ast_get_child(node, "end");
  ASSERT_EQ(end, nullptr);
  allocator_free(allocator, node);
  allocator_free(allocator, stream);

  position = {
      .offset = "abc[:]",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_EXPRESSION_SLICE);
  host = ast_get_child(node, "host");
  ASSERT_EQ(host->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(host, stream, "abc"));
  start = ast_get_child(node, "start");
  ASSERT_EQ(start, nullptr);
  end = ast_get_child(node, "end");
  ASSERT_EQ(end, nullptr);
  allocator_free(allocator, node);
  allocator_free(allocator, stream);
}
TEST_F(test_expression, call) {
  position_t position = {
      .offset = "abc ( a , b )",
      .column = 0,
      .line = 0,
  };
  size_t len = strlen(position.offset);
  token_stream_t stream = create_token_stream(
      allocator, &position, position.offset + len, "./test.cubec");
  ASSERT_NE(stream, nullptr);
  ast_node_t node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_EXPRESSION_CALL);
  ast_node_t host = ast_get_child(node, "callee");
  ASSERT_EQ(host->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(host, stream, "abc"));
  ast_node_t arguments = ast_get_child(node, "arguments");
  ASSERT_EQ(arguments->type, NODE_TYPE_LIST);
  ASSERT_EQ(ast_get_length(arguments), 2);
  ast_node_t argument = ast_get_item(arguments, 0);
  ASSERT_EQ(argument->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(argument, stream, "a"));
  argument = ast_get_item(arguments, 1);
  ASSERT_EQ(argument->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(argument, stream, "b"));
  allocator_free(allocator, node);
  allocator_free(allocator, stream);

  position = {
      .offset = "abc ( )",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_EXPRESSION_CALL);
  host = ast_get_child(node, "callee");
  ASSERT_EQ(host->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(host, stream, "abc"));
  arguments = ast_get_child(node, "arguments");
  ASSERT_EQ(arguments->type, NODE_TYPE_LIST);
  ASSERT_EQ(ast_get_length(arguments), 0);
  allocator_free(allocator, node);
  allocator_free(allocator, stream);

  position = {
      .offset = "abc ( a , b ",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_ERROR);
  allocator_free(allocator, node);
  allocator_free(allocator, stream);

  position = {
      .offset = "abc ( a ,) ",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_ERROR);
  allocator_free(allocator, node);
  allocator_free(allocator, stream);

  position = {
      .offset = "abc (,b) ",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_ERROR);
  allocator_free(allocator, node);
  allocator_free(allocator, stream);
}
TEST_F(test_expression, ptr_declarator) {
  position_t position = {
      .offset = "* i32",
      .column = 0,
      .line = 0,
  };
  size_t len = strlen(position.offset);
  token_stream_t stream = create_token_stream(
      allocator, &position, position.offset + len, "./test.cubec");
  ASSERT_NE(stream, nullptr);
  ast_node_t node = read_expression_atom(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_PTR_DECLARATOR);
  ast_node_t type = ast_get_child(node, "type");
  ASSERT_EQ(type->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(type, stream, "i32"));
  ast_node_t kind = ast_get_child(node, "kind");
  ASSERT_EQ(kind->type, NODE_TYPE_LITERAL_SYMBOL);
  ASSERT_TRUE(node_location_is(kind, stream, "*"));
  allocator_free(allocator, node);
  allocator_free(allocator, stream);

  position = {
      .offset = "[*] i32",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_PTR_DECLARATOR);
  type = ast_get_child(node, "type");
  ASSERT_EQ(type->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(type, stream, "i32"));
  kind = ast_get_child(node, "kind");
  ASSERT_EQ(kind->type, NODE_TYPE_LITERAL_SYMBOL);
  ASSERT_TRUE(node_location_is(kind, stream, "[*]"));
  allocator_free(allocator, node);
  allocator_free(allocator, stream);

  position = {
      .offset = "* const volatile i32",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_PTR_DECLARATOR);
  type = ast_get_child(node, "type");
  ASSERT_EQ(type->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(type, stream, "i32"));
  kind = ast_get_child(node, "kind");
  ASSERT_EQ(kind->type, NODE_TYPE_LITERAL_SYMBOL);
  ASSERT_TRUE(node_location_is(kind, stream, "*"));
  ast_node_t decorators = ast_get_child(node, "decorators");
  ASSERT_EQ(ast_get_length(decorators), 2);
  ast_node_t decor = ast_get_item(decorators, 0);
  ASSERT_TRUE(node_location_is(decor, stream, "const"));
  decor = ast_get_item(decorators, 1);
  ASSERT_TRUE(node_location_is(decor, stream, "volatile"));
  allocator_free(allocator, node);
  allocator_free(allocator, stream);
}

TEST_F(test_expression, array_declarator) {
  position_t position = {
      .offset = "[_]i32",
      .column = 0,
      .line = 0,
  };
  size_t len = strlen(position.offset);
  token_stream_t stream = create_token_stream(
      allocator, &position, position.offset + len, "./test.cubec");
  ASSERT_NE(stream, nullptr);
  ast_node_t node = read_expression_atom(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_ARRAY_DECLARATOR);
  ast_node_t type = ast_get_child(node, "type");
  ASSERT_EQ(type->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(type, stream, "i32"));
  ast_node_t length = ast_get_child(node, "length");
  ASSERT_EQ(length->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(length, stream, "_"));
  allocator_free(allocator, node);
  allocator_free(allocator, stream);

  position = {
      .offset = "[_ i32",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression_atom(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_ERROR);
  allocator_free(allocator, node);
  allocator_free(allocator, stream);

  position = {
      .offset = "[ i32",
      .column = 0,
      .line = 0,
  };
  len = strlen(position.offset);
  stream = create_token_stream(allocator, &position, position.offset + len,
                               "./test.cubec");
  ASSERT_NE(stream, nullptr);
  node = read_expression_atom(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_ERROR);
  allocator_free(allocator, node);
  allocator_free(allocator, stream);
}

TEST_F(test_expression, slice_declarator) {
  position_t position = {
      .offset = "[] i32",
      .column = 0,
      .line = 0,
  };
  size_t len = strlen(position.offset);
  token_stream_t stream = create_token_stream(
      allocator, &position, position.offset + len, "./test.cubec");
  ASSERT_NE(stream, nullptr);
  ast_node_t node = read_expression_atom(allocator, stream);
  ASSERT_EQ(node->type, NODE_TYPE_SLICE_DECLARATOR);
  ast_node_t type = ast_get_child(node, "type");
  ASSERT_EQ(type->type, NODE_TYPE_LITERAL_IDENTIFIER);
  ASSERT_TRUE(node_location_is(type, stream, "i32"));
  allocator_free(allocator, node);
  allocator_free(allocator, stream);
}