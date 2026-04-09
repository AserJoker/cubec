#include "eval/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "engine/error.h"
#include "engine/struct.h"
#include "engine/value.h"
#include "eval/statement_declaration.h"
#include "eval/statement_import.h"
#include <stdalign.h>

cubec_value_t cubec_eval_program(cubec_context_t ctx, cubec_ast_node_t node) {
  cubec_value_t stru = cubec_create_struct_type(ctx, alignof(struct {}), NULL);
  cubec_value_t old = cubec_context_set_eval_context(ctx, stru);
  cubec_ast_node_t statements = cubec_ast_get_child(node, "statements");
  for (size_t idx = 0; idx < cubec_ast_get_length(statements); idx++) {
    cubec_ast_node_t sts = cubec_ast_get_item(statements, idx);
    if (sts->type == CUBEC_NODE_TYPE_STATEMENT_IMPORT) {
      cubec_value_t err = cubec_eval_statement_import(ctx, sts);
      if (cubec_value_is_error(err)) {
        return err;
      }
    } else if (sts->type == CUBEC_NODE_TYPE_STATEMENT_DECLARATION) {
      cubec_value_t err = cubec_eval_statement_declaration(ctx, sts);
      if (cubec_value_is_error(err)) {
        return err;
      }
    } else if (sts->type == CUBEC_NODE_TYPE_STATEMENT_ENUM) {
    } else if (sts->type == CUBEC_NODE_TYPE_STATEMENT_STRUCT) {
    } else if (sts->type == CUBEC_NODE_TYPE_STATEMENT_FUNCTION) {
    } else {
      return cubec_create_compile_error(ctx, sts, "Unsupport top statement");
    }
  }
  cubec_context_set_eval_context(ctx, old);
  return stru;
}