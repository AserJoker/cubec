#include "eval/statement_return.h"
#include "ast/node.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"

cubec_value_t cubec_eval_statement_return(cubec_context_t ctx,
                                          cubec_ast_node_t sts,
                                          const char *filename) {
  if (!ctx->scope_host) {
    return cubec_context_create_compile_error(
        ctx, sts, filename, "Invalid return statement on non-function scope");
  }
  cubec_ast_node_t vnode = cubec_ast_get_child(ctx->allocator, sts, "value");
  if (vnode) {
    cubec_value_t value = cubec_eval_expression(ctx, vnode, filename);
    if (!value) {
      return NULL;
    }
    if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return value;
    }
    ctx->eval_result = value;
  } else {
    ctx->eval_result = ctx->value_undefined;
  }
  return NULL;
}