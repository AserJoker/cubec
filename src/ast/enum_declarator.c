#include "ast/enum_declarator.h"
#include "ast/enum_field.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/literal_keyword.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "reader/token.h"
#include "reader/token_type.h"
ast_node_t read_enum_declarator(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_ENUM_DECLARATOR);
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (token_is(token, TOKEN_TYPE_KEYWORD, "pub")) {
    ast_node_t pub = read_literal_keyword(allocator, stream);
    ast_add_child(allocator, node, "accessor", pub);
    skip_comments(stream);
  }
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_KEYWORD, "enum")) {
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  token = token_stream_get(stream);
  if (token->type == TOKEN_TYPE_IDENTIFIER) {
    ast_node_t identifier = read_literal_identifier(allocator, stream);
    ast_add_child(allocator, node, "identifier", identifier);
    skip_comments(stream);
  }
  token = token_stream_get(stream);
  if (token_is(token, TOKEN_TYPE_SYMBOL, ":")) {
    stream->position++;
    skip_comments(stream);
    ast_node_t type = read_expression_value(allocator, stream);
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
  }
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
  ast_node_t options = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "options", options);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "}")) {
    for (;;) {
      skip_comments(stream);
      ast_node_t field = read_enum_field(allocator, stream);
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
      ast_add_item(options, field);
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
                               stream->filename, "missing ',");
        goto onerror;
      }
    }
  }
  stream->position++;

  node->start = array_get(stream->tokens, position);
  node->end = token_stream_get(stream);
  node->filename = stream->filename;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}