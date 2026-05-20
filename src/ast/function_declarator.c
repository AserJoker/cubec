#include "ast/function_declarator.h"
#include "ast/argument.h"
#include "ast/argument_rest.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/literal_keyword.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement_block.h"
#include "core/allocator.h"
#include "core/location.h"
#include "reader/token.h"
#include "reader/token_type.h"

ast_node_t read_function_declarator(allocator_t allocator,
                                    token_stream_t stream) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_FUNCTION_DECLARATOR);
  ast_node_t err = NULL;
  size_t position = stream->position;
  token_t token = token_stream_get(stream);
  if (token_is(token, TOKEN_TYPE_KEYWORD, "pub")) {
    ast_node_t pub = read_literal_keyword(allocator, stream);
    ast_add_child(allocator, node, "accessor", pub);
    skip_comments(stream);
  }
  token = token_stream_get(stream);
  if (token_is(token, TOKEN_TYPE_KEYWORD, "inline") ||
      token_is(token, TOKEN_TYPE_KEYWORD, "extern") ||
      token_is(token, TOKEN_TYPE_KEYWORD, "comptime")) {
    ast_node_t kind = read_literal_keyword(allocator, stream);
    ast_add_child(allocator, node, "kind", kind);
    skip_comments(stream);
    token = token_stream_get(stream);
  }
  if (!token_is(token, TOKEN_TYPE_KEYWORD, "func")) {
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  ast_node_t closure = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "closure", closure);
  token = token_stream_get(stream);
  if (token_is(token, TOKEN_TYPE_SYMBOL, "|")) {
    stream->position++;
    skip_comments(stream);
    token = token_stream_get(stream);
    if (!token_is(token, TOKEN_TYPE_SYMBOL, "|")) {
      for (;;) {
        skip_comments(stream);
        ast_node_t identifier = read_literal_identifier(allocator, stream);
        if (!identifier) {
          token_t start = array_get(stream->tokens, position);
          token_t end = token_stream_get(stream);
          err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                                 stream->filename, "unexpected expression");
          goto onerror;
        }
        if (identifier->type == NODE_TYPE_ERROR) {
          err = identifier;
          goto onerror;
        }
        ast_add_item(closure, identifier);
        skip_comments(stream);
        token = token_stream_get(stream);
        if (token_is(token, TOKEN_TYPE_SYMBOL, ",")) {
          stream->position++;
        } else if (token_is(token, TOKEN_TYPE_SYMBOL, "|")) {
          break;
        } else {
          token_t start = array_get(stream->tokens, position);
          token_t end = token_stream_get(stream);
          err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                                 stream->filename, "missing ','");
          goto onerror;
        }
      }
    }
    stream->position++;
  }
  skip_comments(stream);
  ast_node_t identifier = read_literal_identifier(allocator, stream);
  if (identifier) {
    if (identifier->type == NODE_TYPE_ERROR) {
      err = identifier;
      goto onerror;
    } else {
      ast_add_child(allocator, node, "identifier", identifier);
    }
  }
  skip_comments(stream);
  token = token_stream_get(stream);
  ast_node_t generics = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "generics", generics);
  if (token_is(token, TOKEN_TYPE_SYMBOL, "[")) {
    stream->position++;
    skip_comments(stream);
    token = token_stream_get(stream);
    if (!token_is(token, TOKEN_TYPE_SYMBOL, "]")) {
      for (;;) {
        skip_comments(stream);
        ast_node_t arg = read_argument_rest(allocator, stream);
        if (!arg) {
          arg = read_argument(allocator, stream);
        }
        if (!arg) {
          token_t start = array_get(stream->tokens, position);
          token_t end = token_stream_get(stream);
          err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                                 stream->filename, "unexpected expression");
          goto onerror;
        }
        if (arg->type == NODE_TYPE_ERROR) {
          err = arg;
          goto onerror;
        }
        ast_add_item(generics, arg);
        skip_comments(stream);
        token = token_stream_get(stream);
        if (token_is(token, TOKEN_TYPE_SYMBOL, ",")) {
          stream->position++;
        } else if (token_is(token, TOKEN_TYPE_SYMBOL, "]")) {
          break;
        } else {
          token_t start = array_get(stream->tokens, position);
          token_t end = token_stream_get(stream);
          err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                                 stream->filename, "missing ','");
          goto onerror;
        }
      }
    }
    stream->position++;
  }
  skip_comments(stream);
  ast_node_t arguments = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "arguments", arguments);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, "(")) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing '('");
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ")")) {
    for (;;) {
      skip_comments(stream);
      ast_node_t arg = read_argument_rest(allocator, stream);
      if (!arg) {
        arg = read_argument(allocator, stream);
      }
      if (!arg) {
        token_t start = array_get(stream->tokens, position);
        token_t end = token_stream_get(stream);
        err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                               stream->filename, "unexpected expression");
        goto onerror;
      }
      if (arg->type == NODE_TYPE_ERROR) {
        err = arg;
        goto onerror;
      }
      ast_add_item(arguments, arg);
      skip_comments(stream);
      token = token_stream_get(stream);
      if (token_is(token, TOKEN_TYPE_SYMBOL, ",")) {
        stream->position++;
      } else if (token_is(token, TOKEN_TYPE_SYMBOL, ")")) {
        break;
      } else {
        token_t start = array_get(stream->tokens, position);
        token_t end = token_stream_get(stream);
        err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                               stream->filename, "missing ','");
        goto onerror;
      }
    }
  }
  stream->position++;
  skip_comments(stream);
  token = token_stream_get(stream);
  if (!token_is(token, TOKEN_TYPE_SYMBOL, ":")) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing ':'");
    goto onerror;
  }
  stream->position++;
  skip_comments(stream);
  ast_node_t type = read_expression_value(allocator, stream);
  if (!type) {
    token_t start = array_get(stream->tokens, position);
    token_t end = token_stream_get(stream);
    err = create_ast_error(allocator, start->loc.begin, end->loc.end,
                           stream->filename, "missing return type");
    goto onerror;
  }
  if (type->type == NODE_TYPE_ERROR) {
    err = type;
    goto onerror;
  }
  ast_add_child(allocator, node, "type", type);
  skip_comments(stream);
  ast_node_t body = read_statement_block(allocator, stream);
  if (body) {
    if (body->type == NODE_TYPE_ERROR) {
      err = body;
      goto onerror;
    } else {
      ast_add_child(allocator, node, "body", body);
    }
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