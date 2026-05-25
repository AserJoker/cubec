
#include "ast/argument_rest.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/literal_keyword.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "reader/token.h"
#include "reader/token_type.h"
ast_node_t read_argument_rest(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_ARGUMENT_REST);
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  ast_node_t mut = NULL;
  if (token_is(token, TOKEN_TYPE_KEYWORD, "const") ||
      token_is(token, TOKEN_TYPE_KEYWORD, "mut")) {
    mut = read_literal_keyword(allocator, stream);
    ast_add_child(allocator, node, "mut", mut);
    skip_comments(stream);
  }
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "...")) {
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  ast_node_t identifier = read_literal_identifier(allocator, stream);
  if (identifier) {
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
    stream->position++;
    skip_comments(stream);
    ast_node_t type = read_expression_value(allocator, stream);
    if (!type) {
      token_t start = array_get(stream->tokens, position);
      token_t end = token_stream_get(stream);
      err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                             stream->filename, "missing type");
      goto onerror;
    }
    if (type->type == NODE_TYPE_ERROR) {
      err = type;
      goto onerror;
    }
    ast_add_child(allocator, node, "type", type);
  } else if (mut) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename,
                           "c-like rest argument cannot be declar with const");
    goto onerror;
  }
  node->start = array_get(stream->tokens, position);
  node->end = token_stream_get(stream);
  node->filename = stream->filename;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}