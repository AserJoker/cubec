#include "ast/expression_binary.h"
#include "ast/expression.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_expression_logical_or(allocator_t allocator,
                                      token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_node_t err = NULL;
  size_t position = stream->position;
  ast_node_t left = read_expression_logical_and(allocator, stream);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  skip_comments(stream);
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "||") ||
      !token_is(token, TOKEN_TYPE_SYMBOL, "??")) {
    err = left;
    goto onerror;
  }
  ast_node_t opt = read_literal_symbol(allocator, stream);
  ast_add_child(allocator, node, "opt", opt);
  skip_comments(stream);
  ast_node_t right = read_expression_logical_or(allocator, stream);
  if (!right) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing right");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}
ast_node_t read_expression_logical_and(allocator_t allocator,
                                       token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_node_t err = NULL;
  size_t position = stream->position;
  ast_node_t left = read_expression_bitwise_or(allocator, stream);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  skip_comments(stream);
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "&&")) {
    err = left;
    goto onerror;
  }
  ast_node_t opt = read_literal_symbol(allocator, stream);
  ast_add_child(allocator, node, "opt", opt);
  skip_comments(stream);
  ast_node_t right = read_expression_logical_and(allocator, stream);
  if (!right) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing right");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}
ast_node_t read_expression_bitwise_or(allocator_t allocator,
                                      token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_node_t err = NULL;
  size_t position = stream->position;
  ast_node_t left = read_expression_bitwise_xor(allocator, stream);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  skip_comments(stream);
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "|")) {
    err = left;
    goto onerror;
  }
  ast_node_t opt = read_literal_symbol(allocator, stream);
  ast_add_child(allocator, node, "opt", opt);
  skip_comments(stream);
  ast_node_t right = read_expression_bitwise_or(allocator, stream);
  if (!right) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing right");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}
ast_node_t read_expression_bitwise_xor(allocator_t allocator,
                                       token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_node_t err = NULL;
  size_t position = stream->position;
  ast_node_t left = read_expression_bitwise_and(allocator, stream);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  skip_comments(stream);
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "^")) {
    err = left;
    goto onerror;
  }
  ast_node_t opt = read_literal_symbol(allocator, stream);
  ast_add_child(allocator, node, "opt", opt);
  skip_comments(stream);
  ast_node_t right = read_expression_bitwise_xor(allocator, stream);
  if (!right) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing right");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}
ast_node_t read_expression_bitwise_and(allocator_t allocator,
                                       token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_node_t err = NULL;
  size_t position = stream->position;
  ast_node_t left = read_expression_equal(allocator, stream);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  skip_comments(stream);
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "&")) {
    err = left;
    goto onerror;
  }
  ast_node_t opt = read_literal_symbol(allocator, stream);
  ast_add_child(allocator, node, "opt", opt);
  skip_comments(stream);
  ast_node_t right = read_expression_bitwise_and(allocator, stream);
  if (!right) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing right");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}
ast_node_t read_expression_equal(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_node_t err = NULL;
  size_t position = stream->position;
  ast_node_t left = read_expression_relation(allocator, stream);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  skip_comments(stream);
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "!=") &&
      !token_is(token, TOKEN_TYPE_SYMBOL, "==")) {
    err = left;
    goto onerror;
  }
  ast_node_t opt = read_literal_symbol(allocator, stream);
  ast_add_child(allocator, node, "opt", opt);
  skip_comments(stream);
  ast_node_t right = read_expression_equal(allocator, stream);
  if (!right) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing right");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}
ast_node_t read_expression_relation(allocator_t allocator,
                                    token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_node_t err = NULL;
  size_t position = stream->position;
  ast_node_t left = read_expression_bitwise_shift(allocator, stream);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  skip_comments(stream);
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ">") &&
      !token_is(token, TOKEN_TYPE_SYMBOL, ">=") &&
      !token_is(token, TOKEN_TYPE_SYMBOL, "<") &&
      !token_is(token, TOKEN_TYPE_SYMBOL, "<=")) {
    err = left;
    goto onerror;
  }
  ast_node_t opt = read_literal_symbol(allocator, stream);
  ast_add_child(allocator, node, "opt", opt);
  skip_comments(stream);
  ast_node_t right = read_expression_relation(allocator, stream);
  if (!right) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing right");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}
ast_node_t read_expression_bitwise_shift(allocator_t allocator,
                                         token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_node_t err = NULL;
  size_t position = stream->position;
  ast_node_t left = read_expression_additive(allocator, stream);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  skip_comments(stream);
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ">>") &&
      !token_is(token, TOKEN_TYPE_SYMBOL, "<<")) {
    err = left;
    goto onerror;
  }
  ast_node_t opt = read_literal_symbol(allocator, stream);
  ast_add_child(allocator, node, "opt", opt);
  skip_comments(stream);
  ast_node_t right = read_expression_bitwise_shift(allocator, stream);
  if (!right) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing right");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}
ast_node_t read_expression_additive(allocator_t allocator,
                                    token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_node_t err = NULL;
  size_t position = stream->position;
  ast_node_t left = read_expression_multiplicative(allocator, stream);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  skip_comments(stream);
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "+") &&
      !token_is(token, TOKEN_TYPE_SYMBOL, "-")) {
    err = left;
    goto onerror;
  }
  ast_node_t opt = read_literal_symbol(allocator, stream);
  ast_add_child(allocator, node, "opt", opt);
  skip_comments(stream);
  ast_node_t right = read_expression_additive(allocator, stream);
  if (!right) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing right");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}
ast_node_t read_expression_multiplicative(allocator_t allocator,
                                          token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_node_t err = NULL;
  size_t position = stream->position;
  ast_node_t left = read_expression_prefix(allocator, stream);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  skip_comments(stream);
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "*") &&
      !token_is(token, TOKEN_TYPE_SYMBOL, "/") &&
      !token_is(token, TOKEN_TYPE_SYMBOL, "%")) {
    err = left;
    goto onerror;
  }
  ast_node_t opt = read_literal_symbol(allocator, stream);
  ast_add_child(allocator, node, "opt", opt);
  skip_comments(stream);
  ast_node_t right = read_expression_multiplicative(allocator, stream);
  if (!right) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing right");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}
ast_node_t read_expression_prefix(allocator_t allocator,
                                  token_stream_t stream) {
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "+") &&
      !token_is(token, TOKEN_TYPE_SYMBOL, "-") &&
      !token_is(token, TOKEN_TYPE_SYMBOL, "!") &&
      !token_is(token, TOKEN_TYPE_SYMBOL, "~") &&
      !token_is(token, TOKEN_TYPE_SYMBOL, "*") &&
      !token_is(token, TOKEN_TYPE_SYMBOL, "&")) {
    return read_expression_value(allocator, stream);
  }
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_node_t err = NULL;
  size_t position = stream->position;
  ast_node_t opt = read_literal_symbol(allocator, stream);
  ast_add_child(allocator, node, "opt", opt);
  skip_comments(stream);
  ast_node_t right = read_expression_prefix(allocator, stream);
  if (!right) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing right");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}