#include "ast/expression_assigment.h"
#include "ast/expression.h"
#include "ast/expression_binary.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

cubec_ast_node_t
cubec_read_ast_expression_assigment(cubec_allocator_t allocator,
                                    cubec_position_t *position, const char *end,
                                    const char *filename) {
  static const char *opts[] = {
      "=",  "+=", "-=", "*=",  "/=",  "%=",  ">>=", "<<=",
      "&=", "|=", "^=", "&&=", "||=", "??=", NULL,
  };
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_ASSIGMENT);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t identifier = NULL;
  if (*current.offset == '*') {
    identifier = cubec_read_ast_expression_binary_prefix(allocator, &current,
                                                         end, filename);
  } else {
    identifier =
        cubec_read_ast_expression18(allocator, &current, end, filename);
  }
  if (!identifier) {
    goto onerror;
  }
  if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "identifier", identifier);
  if (identifier->type != CUBEC_NODE_TYPE_LITERAL_IDENTIFIER &&
      identifier->type != CUBEC_NODE_TYPE_EXPRESSION_MEMBER &&
      identifier->type != CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER &&
      identifier->type != CUBEC_NODE_TYPE_EXPRESSION_BINARY) {
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);

  cubec_ast_node_t opt =
      cubec_read_ast_literal_symbol(allocator, &current, end, filename);
  if (!opt) {
    goto onerror;
  }
  if (opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "opt", opt);
  size_t idx = 0;
  while (opts[idx] != NULL) {
    if (cubec_location_is(opt->loc, opts[idx])) {
      break;
    }
    idx++;
  }
  if (opts[idx] == NULL) {
    goto onerror;
  }

  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }

  cubec_ast_node_t value =
      cubec_read_ast_expression3(allocator, &current, end, filename);
  if (!value) {
    err = cubec_create_ast_error(
        allocator, *position, current, filename,
        "Invalid or unexpected token, missing initialize expression");
    goto onerror;
  }
  if (value->type == CUBEC_NODE_TYPE_ERROR) {
    err = value;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "value", value);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}