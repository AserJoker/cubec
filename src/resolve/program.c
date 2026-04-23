#include "resolve/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/value.h"
#include "resolve/statement_declaration.h"
#include <inttypes.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

value_t resolve_program(context_t ctx, ast_node_t node) {
  ast_node_t statements = ast_get_child(node, "statements");
  allocator_t allocator = context_get_allocator(ctx);
  for (size_t idx = 0; idx < ast_get_length(statements); idx++) {
    ast_node_t sts = ast_get_item(statements, idx);
    value_t err = NULL;
    if (sts->type == NODE_TYPE_STATEMENT_DECLARATION) {
      err = resolve_statement_declaration(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_FUNCTION) {
    } else if (sts->type == NODE_TYPE_STATEMENT_STRUCT) {
    } else if (sts->type == NODE_TYPE_STATEMENT_ENUM) {
    }
    if (err && value_is_error(err)) {
      return err;
    }
  }
  return context_get_undefined(ctx);
}