#include "ast/expression_assigment.h"
#include "ast/expression.h"
#include "ast/expression_binary.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

ast_node_t read_ast_expression_assigment(allocator_t allocator,
                                         position_t *position, const char *end,
                                         const char *filename) {
  static const char *opts[] = {
      "=",  "+=", "-=", "*=",  "/=",  "%=",  ">>=", "<<=",
      "&=", "|=", "^=", "&&=", "||=", "??=", NULL,
  };
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_ASSIGMENT);
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t identifier = NULL;
  if (*current.offset == '*') {
    identifier =
        read_ast_expression_binary_prefix(allocator, &current, end, filename);
  } else {
    identifier = read_ast_expression18(allocator, &current, end, filename);
  }
  if (!identifier) {
    goto onerror;
  }
  if (identifier->type == NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  ast_add_child(allocator, node, "identifier", identifier);
  if (identifier->type != NODE_TYPE_LITERAL_IDENTIFIER &&
      identifier->type != NODE_TYPE_EXPRESSION_MEMBER &&
      identifier->type != NODE_TYPE_EXPRESSION_COMPUTE_MEMBER &&
      identifier->type != NODE_TYPE_EXPRESSION_BINARY) {
    goto onerror;
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  allocator_free(allocator, err);

  ast_node_t opt = read_ast_literal_symbol(allocator, &current, end, filename);
  if (!opt) {
    goto onerror;
  }
  if (opt->type == NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  ast_add_child(allocator, node, "opt", opt);
  size_t idx = 0;
  while (opts[idx] != NULL) {
    if (location_is(opt->loc, opts[idx])) {
      break;
    }
    idx++;
  }
  if (opts[idx] == NULL) {
    goto onerror;
  }

  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }

  ast_node_t value = read_ast_expression3(allocator, &current, end, filename);
  if (!value) {
    err = create_ast_error(
        allocator, *position, current, filename,
        "invalid or unexpected token, missing initialize expression");
    goto onerror;
  }
  if (value->type == NODE_TYPE_ERROR) {
    err = value;
    goto onerror;
  }
  ast_add_child(allocator, node, "value", value);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}