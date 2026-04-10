#include "eval/statement_function.h"
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#include "eval/function_declaratior.h"

cubec_value_t cubec_eval_statement_function(cubec_context_t ctx,
                                            cubec_ast_node_t node) {
  cubec_ast_node_t function = cubec_ast_get_child(node, "function");
  cubec_value_t err = cubec_eval_function_declaratior(ctx, function);
  if (cubec_value_is_error(err)) {
    return err;
  }
  return cubec_context_get_undefined(ctx);
}