#include "ast/initialize_list.h"
#include "ast/expression.h"
#include "ast/expression_spread.h"
#include "ast/initialize_field.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
#include "reader/token.h"
#include "reader/token_type.h"
ast_node_t read_initialize_list(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_INITIALIZE_LIST);
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ".")) {
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "{")) {
    ast_node_t type = read_expression_value(allocator, stream);
    if (type) {
      if (type->type == NODE_TYPE_ERROR) {
        err = type;
        goto onerror;
      }
      ast_add_child(allocator, node, "type", type);
    }
  }
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "{")) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing '{'");
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  token = token_stream_get(stream);
  ast_node_t fields = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "fields", fields);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "}")) {
    for (;;) {
      skip_comments(stream);
      ast_node_t field = read_initialize_field(allocator, stream);
      if (!field) {
        field = read_expression_single(allocator, stream);
      }
      if (!field) {
        field = read_expression_spread(allocator, stream);
      }
      if (!field) {
        token_t start = array_get(stream->tokens, position);
        token_t end = token_stream_get(stream);
        err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                               stream->filename, "unexpected expression");
        goto onerror;
      }
      if (field->type == NODE_TYPE_ERROR) {
        err = field;
        goto onerror;
      }
      ast_add_item(fields, field);
      skip_comments(stream);
      token = token_stream_get(stream);
      if (token_is(token, TOKEN_TYPE_SYMBOL, ",")) {
        stream->position++;
      } else if (token_is(token, TOKEN_TYPE_SYMBOL, "}")) {
        break;
      } else {
        token_t start = array_get(stream->tokens, position);
        token_t end = token_stream_get(stream);
        err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                               stream->filename, "missing '}'");
        goto onerror;
      }
    }
  }
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}