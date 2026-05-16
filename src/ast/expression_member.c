#include "ast/expression_member.h"
#include "ast/literal_identifier.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_expression_member(allocator_t allocator,
                                  token_stream_t stream) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ".")) {
    goto onerror;
  }
  stream->position++;
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_MEMBER);
  skip_comments(stream);
  token = token_stream_get(stream);
  ast_node_t field = NULL;
  if (token_is(token, TOKEN_TYPE_SYMBOL, "*") ||
      token_is(token, TOKEN_TYPE_SYMBOL, "&")) {
    field = read_literal_symbol(allocator, stream);
  } else {
    field = read_literal_identifier(allocator, stream);
  }
  if (!field) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing field");
    goto onerror;
  }
  ast_add_child(allocator, node, "field", field);
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}
