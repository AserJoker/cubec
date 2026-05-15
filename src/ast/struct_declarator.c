#include "ast/struct_declarator.h"
#include "ast/literal_identifier.h"
#include "ast/literal_keyword.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement_declaration.h"
#include "ast/statement_enum.h"
#include "ast/statement_function.h"
#include "ast/statement_struct.h"
#include "ast/struct_field.h"
#include "core/allocator.h"
#include "core/location.h"
#include "reader/token.h"
#include "reader/token_type.h"
/**
[[packed]]
[[aligned(4)]]
*/
ast_node_t read_struct_declarator(allocator_t allocator,
                                  token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STRUCT_DECLARATOR);
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (token_is(token, TOKEN_TYPE_KEYWORD, "pub")) {
    ast_node_t pub = read_literal_keyword(allocator, stream);
    ast_add_child(allocator, node, "pub", pub);
    skip_comments(stream);
  }
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_KEYWORD, "struct")) {
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
  ast_node_t fields = create_ast_node(allocator, NODE_TYPE_LIST);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "}")) {
    for (;;) {
      skip_comments(stream);
      ast_node_t field = read_statement_declaration(allocator, stream);
      if (!field) {
        field = read_statement_function(allocator, stream);
      }
      if (!field) {
        field = read_statement_struct(allocator, stream);
      }
      if (!field) {
        field = read_statement_enum(allocator, stream);
      }
      if (!field) {
        field = read_struct_field(allocator, stream);
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
      if (token_is(token, TOKEN_TYPE_SYMBOL, "}")) {
        break;
      }
    }
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