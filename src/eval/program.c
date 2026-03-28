#include "eval/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/array.h"
#include "core/map.h"
#include "engine/context.h"
#include "engine/value.h"
#include "eval/statement_test.h"

cubec_value_t cubec_eval_program(cubec_context_t ctx, cubec_ast_node_t program,
                                 const char *filename) {
  cubec_ast_node_t statements =
      cubec_map_get(program->children, "statements", NULL);
  for (size_t idx = 0; idx < cubec_array_get_size(statements->items); idx++) {
    cubec_ast_node_t node = cubec_array_get(statements->items, idx);
    if (node->type == CUBEC_NODE_TYPE_STATEMENT_TEST) {
      cubec_eval_statement_test(ctx, node, filename);
    } else {
      return cubec_context_create_compile_error(ctx, node, filename,
                                                "Unsupport top statement");
    }
  }
  return ctx->value_undefined;
}