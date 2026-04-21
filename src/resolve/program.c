#include "resolve/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/statement_declaration.h"
#include <stdalign.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

value_t resolve_program(context_t ctx, ast_node_t node) {
  size_t len = strlen(node->loc.filename);
  char id[len + 16];
  sprintf(id, "module(%s)", node->loc.filename);
  type_t module_struct = create_struct_type(ctx, NULL, id, alignof(struct {}));
  value_t module = create_type_value(ctx, module_struct, false, true, NULL);
  value_t current = context_set_binding(ctx, module);
  ast_node_t statements = ast_get_child(node, "statements");
  context_push_scope(ctx);
  for (size_t idx = 0; idx < ast_get_length(statements); idx++) {
    ast_node_t sts = ast_get_item(statements, idx);
    if (sts->type == NODE_TYPE_STATEMENT_IMPORT) {
    } else if (sts->type == NODE_TYPE_STATEMENT_DECLARATION) {
      value_t err = resolve_statement_declaration(ctx, sts);
      if (value_is_error(err)) {
        if (context_is_comptime(ctx)) {
          return err;
        } else {
          fprintf(stderr, "%s\n", error_get_message(err));
        }
      }
      if (value_is_interrupt(err)) {
        return err;
      }
    } else if (sts->type == NODE_TYPE_STATEMENT_FUNCTION) {
    } else if (sts->type == NODE_TYPE_STATEMENT_STRUCT) {
    } else if (sts->type == NODE_TYPE_STATEMENT_ENUM) {
    } else {
      value_t err = create_compile_error(ctx, sts, "invalid top statement");
      if (value_is_error(err)) {
        if (context_is_comptime(ctx)) {
          return err;
        } else {
          fprintf(stderr, "%s\n", error_get_message(err));
        }
      }
    }
  }
  context_pop_scope(ctx);
  context_set_binding(ctx, current);
  return module;
}