#include "ast/ptr_declarator.h"
#include "ast/expression.h"
#include "ast/literal_keyword.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_ptr_declarator(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "*") &&
      !token_is(token, TOKEN_TYPE_SYMBOL, "[*]")) {
    goto onerror;
  }
  node = create_ast_node(allocator, NODE_TYPE_PTR_DECLARATOR);
  ast_node_t kind = read_literal_symbol(allocator, stream);
  ast_add_child(allocator, node, "kind", kind);
  skip_comments(stream);
  ast_node_t decorators = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "decorators", decorators);
  for (;;) {
    skip_comments(stream);
    token = token_stream_get(stream);
    if (token_is(token, TOKEN_TYPE_KEYWORD, "const") ||
        token_is(token, TOKEN_TYPE_KEYWORD, "volatile")) {
      ast_node_t dec = read_literal_keyword(allocator, stream);
      ast_add_item(decorators, dec);
    } else {
      break;
    }
  }
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
  node->start = position;
  node->end = stream->position;
  return node;
onerror:
  allocator_free(allocator, node);
  stream->position = position;
  return err;
}