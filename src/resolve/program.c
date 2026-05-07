#include "resolve/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
#include "resolve/statement_declaration.h"
#include "resolve/statement_function.h"
#include "resolve/statement_struct.h"
#include "resolve/statement_test.h"
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
      err = resolve_statement_function(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_STRUCT) {
      err = resolve_statement_struct(ctx, sts);
    } else if (sts->type == NODE_TYPE_STATEMENT_TEST) {
      err = resolve_statement_test(ctx, sts);
    } else {
      err = create_comptime_error(ctx, sts, "invalid top statement");
    }
    if (err && value_is_error(err)) {
      context_push_error(ctx, err);
    }
  }
  return context_get_undefined(ctx);
}