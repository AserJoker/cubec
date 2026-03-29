#include "eval/statement_function.h"
#include "ast/node.h"
#include "engine/type.h"
#include "eval/function_declarator.h"

cubec_value_t cubec_eval_statement_function(cubec_context_t ctx,
                                            cubec_ast_node_t sts,
                                            const char *filename) {
  cubec_ast_node_t function =
      cubec_ast_get_child(ctx->allocator, sts, "function");
  cubec_value_t err = cubec_eval_function_declarator(ctx, function, filename);
  if (!err || err->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return err;
  }
  return ctx->value_undefined;
}