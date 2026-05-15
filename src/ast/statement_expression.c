#include "ast/statement_expression.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_statement_expression(allocator_t allocator,
                                     token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STATEMENT_EXPRESSION);
  ast_node_t err = NULL;
  size_t position = stream->position;
  ast_node_t expression = read_expression(allocator, stream);
  if (!expression) {
    goto onerror;
  }
  if (expression->type == NODE_TYPE_ERROR) {
    err = expression;
    goto onerror;
  }
  ast_add_child(allocator, node, "expression", expression);
  skip_comments(stream);
  token_t token = token_stream_get(stream);
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