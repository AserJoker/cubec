#include "ast/statement_import.h"
#include "ast/literal_identifier.h"
#include "ast/literal_string.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "reader/token.h"
#include "reader/token_type.h"
// import
// [comment]
// (name:literal_identifier)
// [comment]
// from
// [comment]
// (source:literal_string)
// [comment]
// ;
ast_node_t read_statement_import(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_KEYWORD, "import")) {
    return NULL;
  }
  stream->position++;
  node = create_ast_node(allocator, NODE_TYPE_STATEMENT_IMPORT);
  skip_comments(stream);
  ast_node_t identifier = read_literal_identifier(allocator, stream);
  if (!identifier) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing identifier");
    goto onerror;
  }
  if (identifier->type == NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  ast_add_child(allocator, node, "identifier", identifier);
  skip_comments(stream);
  token = token_stream_get(stream);
  if (token_is(token, TOKEN_TYPE_KEYWORD, "from")) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing 'from'");
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  ast_node_t source = read_literal_string(allocator, stream);
  if (!source) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing source");
    goto onerror;
  }
  if (source->type == NODE_TYPE_ERROR) {
    err = source;
    goto onerror;
  }
  ast_add_child(allocator, node, "source", source);
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ";")) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing ';'");
    goto onerror;
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