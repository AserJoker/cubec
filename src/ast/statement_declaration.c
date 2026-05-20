#include "ast/statement_declaration.h"
#include "ast/literal_keyword.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/variable_declarator.h"
#include "core/allocator.h"
#include "core/location.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_statement_declaration(allocator_t allocator,
                                      token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STATEMENT_DECLARATION);
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (token_is(token, TOKEN_TYPE_KEYWORD, "pub")) {
    ast_node_t pub = read_literal_keyword(allocator, stream);
    ast_add_child(allocator, node, "accessor", pub);
    skip_comments(stream);
  }
  token = token_stream_get(stream);
  if (token_is(token, TOKEN_TYPE_KEYWORD, "extern") ||
      token_is(token, TOKEN_TYPE_KEYWORD, "comptime")) {
    ast_node_t keyword = read_literal_keyword(allocator, stream);
    if (!keyword) {
      goto onerror;
    }
    ast_add_child(allocator, node, "kind", keyword);
    skip_comments(stream);
  }
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_KEYWORD, "let") &&
      !token_is(token, TOKEN_TYPE_KEYWORD, "const")) {
    goto onerror;
  }
  ast_node_t type = read_literal_keyword(allocator, stream);
  ast_add_child(allocator, node, "type", type);
  skip_comments(stream);
  ast_node_t declarations = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "declarations", declarations);
  for (;;) {
    skip_comments(stream);
    ast_node_t declar = read_variable_declarator(allocator, stream);
    if (!declar) {
      token_t start = array_get(stream->tokens, position);
      token_t end = token_stream_get(stream);
      err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                             stream->filename, "missing declaration");
      goto onerror;
    }
    if (declar->type == NODE_TYPE_ERROR) {
      err = declar;
      goto onerror;
    }
    ast_add_item(declarations, declar);
    skip_comments(stream);
    token = token_stream_get(stream);
    if (token_is(token, TOKEN_TYPE_SYMBOL, ",")) {
      stream->position++;
    } else if (token_is(token, TOKEN_TYPE_SYMBOL, ";")) {
      break;
    } else {
      token_t start = array_get(stream->tokens, position);
      token_t end = token_stream_get(stream);
      err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                             stream->filename, "missing ','");
      goto onerror;
    }
  }
  stream->position++;

  node->start = array_get(stream->tokens, position);
  node->end = token_stream_get(stream);
  node->filename = stream->filename;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}