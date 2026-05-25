
#include "ast/callable_argument.h"
#include "ast/expression.h"
#include "ast/literal_keyword.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "reader/token.h"
#include "reader/token_type.h"
// [const|mutable] (name identifier) : (expression)
ast_node_t read_callable_argument(allocator_t allocator,
                                  token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_CALLABLE_ARGUMENT);
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (token_is(token, TOKEN_TYPE_KEYWORD, "const") ||
      token_is(token, TOKEN_TYPE_KEYWORD, "mut")) {
    ast_node_t mutable = read_literal_keyword(allocator, stream);
    ast_add_child(allocator, node, "mut", mutable);
    skip_comments(stream);
  }
  skip_comments(stream);
  ast_node_t type = read_expression_value(allocator, stream);
  if (!type) {
    goto onerror;
  }
  if (type->type == NODE_TYPE_ERROR) {
    err = type;
    goto onerror;
  }
  ast_add_child(allocator, node, "type", type);
  node->start = array_get(stream->tokens, position);
  node->end = token_stream_get(stream);
  node->filename = stream->filename;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}