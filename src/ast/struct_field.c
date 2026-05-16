#include "ast/struct_field.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/literal_keyword.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_struct_field(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STRUCT_FIELD);
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (token_is(token, TOKEN_TYPE_KEYWORD, "pub")) {
    ast_node_t pub = read_literal_keyword(allocator, stream);
    ast_add_child(allocator, node, "accessor", pub);
    skip_comments(stream);
  }
  token = token_stream_get(stream);
  ast_node_t identifier = read_literal_identifier(allocator, stream);
  if (!identifier) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "unexpected expression");
    goto onerror;
  }
  if (identifier->type == NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  ast_add_child(allocator, node, "identifier", identifier);
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ":")) {

    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing ':'");
    goto onerror;
  }
  skip_comments(stream);
  ast_node_t type = read_expression_single(allocator, stream);
  if (!type) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "unexpected expression");
    goto onerror;
  }
  if (type->type == NODE_TYPE_ERROR) {
    err = type;
    goto onerror;
  }
  ast_add_child(allocator, node, "type", type);
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ";")) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing ';'");
    goto onerror;
  }
  stream->position++;
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}