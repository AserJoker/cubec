#include "ast/switch_match.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement_block.h"
#include "core/allocator.h"
#include "core/location.h"
#include "reader/token.h"
/**
  ((expression[,expression]*)?)->statement
*/
ast_node_t read_switch_match(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_SWITCH_MATCH);
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "(")) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err =
        create_ast_error(allocator, start->loc.begin, end->loc.end,
                         stream->filename, "invalid swich match, missing '('");
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  token = token_stream_get(stream);
  ast_node_t items = create_ast_node(allocator, NODE_TYPE_LIST);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ")")) {
    for (;;) {
      skip_comments(stream);
      ast_node_t expr = read_expression_single(allocator, stream);
      if (!expr) {
        token_t start = array_get(stream->tokens, position);
        token_t end = token_stream_get(stream);
        err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                               stream->filename, "unexpected expression");
        goto onerror;
      }
      if (expr->type == NODE_TYPE_ERROR) {
        err = expr;
        goto onerror;
      }
      ast_add_item(items, expr);
      skip_comments(stream);
      token = token_stream_get(stream);
      if (token_is(token, TOKEN_TYPE_SYMBOL, ",")) {
        stream->position++;
      } else if (token_is(token, TOKEN_TYPE_SYMBOL, ")")) {
        break;
      } else {
        token_t start = array_get(stream->tokens, position);
        token_t end = token_stream_get(stream);
        err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                               stream->filename, "unexpected expression");
        goto onerror;
      }
    }
  }
  stream->position++;
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "->")) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing '->'");
    goto onerror;
  }
  ast_node_t body = read_statement_block(allocator, stream);
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

  node->start = array_get(stream->tokens, position);
  node->end = token_stream_get(stream);
  node->filename = stream->filename;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}