#include "ast/statement_return.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_statement_return(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STATEMENT_RETURN);
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_KEYWORD, "return")) {
    goto onerror;
  }
  skip_comments(stream);
  ast_node_t value = read_expression(allocator, stream);
  if (value) {
    if (value->type == NODE_TYPE_ERROR) {
      err = value;
      goto onerror;
    }
    ast_add_child(allocator, node, "value", value);
  }
  skip_comments(stream);
  token = token_stream_get(stream);
  if (value && value->type != NODE_TYPE_FUNCTION_DECLARATOR &&
      value->type != NODE_TYPE_STRUCT_DECLARATOR &&
      value->type != NODE_TYPE_ENUM_DECLARATOR &&
      value->type != NODE_TYPE_INITIALIZE_LIST) {
    if (!token_is(token, TOKEN_TYPE_SYMBOL, ";")) {
      token_t start = array_get(stream->tokens, position);
      token_t end = token_stream_get(stream);
      err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                             stream->filename, "missing ';'");
      goto onerror;
    }
  }
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