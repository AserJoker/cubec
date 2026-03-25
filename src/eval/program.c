#include "eval/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/list.h"
#include "engine/context.h"
#include "engine/value.h"
#include "eval/statement_test.h"

cubec_value_t cubec_eval_program(cubec_context_t ctx,
                                 cubec_ast_program_t program,
                                 const char *filename) {
  cubec_ast_list_node_t list = (cubec_ast_list_node_t)program->statements;
  for (cubec_list_node_t it = cubec_list_get_first(list->items);
       it != cubec_list_get_end(list->items); it = cubec_list_node_next(it)) {
    cubec_ast_node_t node = cubec_list_node_get(it);
    if (node->type == CUBEC_NODE_TYPE_STATEMENT_TEST) {
      cubec_eval_statement_test(ctx, (cubec_ast_statement_test_t)node,
                                filename);
    } else {
      return cubec_context_create_compile_error(ctx, node, filename,
                                                "Unsupport top statement");
    }
  }
  return ctx->value_undefined;
}