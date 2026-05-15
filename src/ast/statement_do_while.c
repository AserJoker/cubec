#include "ast/statement_do_while.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_statement_do_while(allocator_t allocator,
                                   token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STATEMENT_DO_WHILE);
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_KEYWORD, "do")) {
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  ast_node_t body = read_statement(allocator, stream);
  if (!body) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "unexpected expression");
    goto onerror;
  }
  ast_add_child(allocator, node, "body", body);
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_KEYWORD, "while")) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing 'while'");
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "(")) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing '('");
    goto onerror;
  }
  skip_comments(stream);
  ast_node_t condition = read_expression(allocator, stream);
  if (!condition) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "unexpected expression");
    goto onerror;
  }
  if (condition->type == NODE_TYPE_ERROR) {
    err = condition;
    goto onerror;
  }
  ast_add_child(allocator, node, "condition", condition);
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ")")) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing ')'");
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
