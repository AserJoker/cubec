#include "ast/statement_enum.h"
#include "ast/enum_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_statement_enum(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STATEMENT_ENUM);
  ast_node_t err = NULL;
  size_t position = stream->position;
  ast_node_t enu = read_enum_declarator(allocator, stream);
  if (!enu) {
    goto onerror;
  }
  if (enu->type == NODE_TYPE_ERROR) {
    err = enu;
    goto onerror;
  }
  ast_add_child(allocator, node, "enum", enu);
  skip_comments(stream);
  token_t token = token_stream_get(stream);
  if (token_is(token, TOKEN_TYPE_SYMBOL, ";")) {
    stream->position++;
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