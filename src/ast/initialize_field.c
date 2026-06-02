#include "ast/initialize_field.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "reader/token.h"
#include "reader/token_type.h"
ast_node_t read_initialize_field(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ".")) {
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  node = create_ast_node(allocator, NODE_TYPE_INITIALIZE_FIELD);
  ast_node_t identifier = read_literal_identifier(allocator, stream);
  if (!identifier) {
    goto onerror;
  }
  ast_add_child(allocator, node, "identifier", identifier);
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "=")) {
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  ast_node_t value = read_expression_single(allocator, stream);
  if (!value) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "unexpected expression");
    goto onerror;
  }
  if (value->type == NODE_TYPE_ERROR) {
    err = value;
    goto onerror;
  }
  ast_add_child(allocator, node, "value", value);

  node->start = array_get(stream->tokens, position);
  node->end = token_stream_get(stream);
  node->filename = stream->filename;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}