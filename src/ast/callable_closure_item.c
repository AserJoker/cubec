#include "ast/callable_closure_item.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_callable_closure_item(allocator_t allocator,
                                      token_stream_t stream) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  size_t position = stream->position;
  ast_node_t identifier = read_literal_identifier(allocator, stream);
  if (!identifier) {
    goto onerror;
  }
  if (identifier->type == NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  node = create_ast_node(allocator, NODE_TYPE_CALLABLE_CLOSURE_ITEM);
  ast_add_child(allocator, node, "identifier", identifier);
  skip_comments(stream);
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ":")) {
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  ast_node_t type = read_expression_value(allocator, stream);
  if (!type) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "invalid closure type");
    goto onerror;
  }
  ast_add_child(allocator, node, "type", type);

  node->start = array_get(stream->tokens, position);
  node->end = token_stream_get(stream);
  node->filename = stream->filename;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}