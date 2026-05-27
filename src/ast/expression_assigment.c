#include "ast/expression_assigment.h"
#include "ast/expression_binary.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_expression_assigment(allocator_t allocator,
                                     token_stream_t stream) {
  static const char *opts[] = {
      "=",  "+=", "-=", "*=",  "/=",  "%=",  ">>=", "<<=",
      "&=", "|=", "^=", "&&=", "||=", "??=", NULL,
  };
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  size_t position = stream->position;
  ast_node_t identifier = read_expression_logical_or(allocator, stream);
  if (!identifier) {
    goto onerror;
  }
  if (identifier->type == NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  skip_comments(stream);
  token_t token = token_stream_get(stream);
  ast_node_t opt = NULL;
  if (token->type == TOKEN_TYPE_SYMBOL) {
    for (size_t idx = 0; opts[idx] != 0; idx++) {
      if (token_is(token, TOKEN_TYPE_SYMBOL, opts[idx])) {
        opt = read_literal_symbol(allocator, stream);
        break;
      }
    }
  }
  if (!opt) {
    return identifier;
  }
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_ASSIGMENT);
  ast_add_child(allocator, node, "left", identifier);
  ast_add_child(allocator, node, "opt", opt);
  skip_comments(stream);
  ast_node_t value = read_expression_assigment(allocator, stream);
  if (!value) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing initialize");
    goto onerror;
  }
  if (value->type == NODE_TYPE_ERROR) {
    err = value;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", value);

  node->start = array_get(stream->tokens, position);
  node->end = token_stream_get(stream);
  node->filename = stream->filename;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}