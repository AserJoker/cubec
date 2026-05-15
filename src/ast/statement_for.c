#include "ast/statement_for.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement.h"
#include "ast/statement_declaration.h"
#include "ast/statement_empty.h"
#include "ast/statement_expression.h"
#include "core/allocator.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_statement_for(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STATEMENT_FOR);
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_KEYWORD, "for")) {
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
  stream->position++;
  skip_comments(stream);
  ast_node_t init = read_statement_declaration(allocator, stream);
  if (!init) {
    init = read_statement_expression(allocator, stream);
  }
  if (!init) {
    init = read_statement_empty(allocator, stream);
  }
  if (!init) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "unexpected expression");
    goto onerror;
  }
  if (init->type == NODE_TYPE_ERROR) {
    err = init;
    goto onerror;
  }
  ast_add_child(allocator, node, "init", init);
  skip_comments(stream);
  ast_node_t conditon = read_statement_expression(allocator, stream);
  if (!conditon) {
    conditon = read_statement_empty(allocator, stream);
  }
  if (!conditon) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "unexpected expression");
    goto onerror;
  }
  if (conditon->type == NODE_TYPE_ERROR) {
    err = conditon;
    goto onerror;
  }
  ast_add_child(allocator, node, "condition", conditon);
  skip_comments(stream);
  ast_node_t after = read_expression(allocator, stream);
  if (after) {
    if (after->type == NODE_TYPE_ERROR) {
      err = after;
      goto onerror;
    }
    ast_add_child(allocator, node, "after", after);
  }
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ")")) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "unexpected expression");
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
  if (body->type == NODE_TYPE_ERROR) {
    err = body;
    goto onerror;
  }
  ast_add_child(allocator, node, "body", body);
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}