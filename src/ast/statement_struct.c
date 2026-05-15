#include "ast/statement_struct.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/struct_declarator.h"
#include "core/allocator.h"
#include "core/position.h"
#include "reader/token.h"

ast_node_t read_statement_struct(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STATEMENT_STRUCT);
  ast_node_t err = NULL;
  size_t position = stream->position;
  ast_node_t stru = read_struct_declarator(allocator, stream);
  if (!stru) {
    goto onerror;
  }
  if (stru->type == NODE_TYPE_ERROR) {
    err = stru;
    goto onerror;
  }
  ast_add_child(allocator, node, "struct", stru);
  skip_comments(stream);
  token_t token = token_stream_get(stream);
  if (token_is(token, TOKEN_TYPE_SYMBOL, ";")) {
    stream->position++;
  }
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}