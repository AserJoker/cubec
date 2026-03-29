#include "eval/function_body.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "engine/context.h"
#include "engine/scope.h"
#include "eval/statement_block.h"
#include "eval/statement_expression.h"
#include "eval/statement_function.h"
#include "eval/statement_return.h"
#include <stdbool.h>
cubec_value_t cubec_eval_function_body(cubec_context_t ctx,
                                       cubec_ast_node_t body,
                                       const char *filename) {
  cubec_scope_t parent = ctx->current;
  cubec_context_push_scope(ctx);
  cubec_ast_node_t statements =
      cubec_ast_get_child(ctx->allocator, body, "statements");
  cubec_value_t result = ctx->value_undefined;
  for (size_t idx = 0; idx < cubec_array_get_size(statements->items); idx++) {
    cubec_ast_node_t node = cubec_array_get(statements->items, idx);
    if (node->type == CUBEC_NODE_TYPE_STATEMENT_EXPRESSION) {
      cubec_value_t err = cubec_eval_statement_expression(ctx, node, filename);
      if (!err) {
        result = ctx->eval_result;
        break;
      }
      if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
        return err;
      }
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_BLOCK) {
      cubec_value_t err = cubec_eval_statement_block(ctx, node, filename);
      if (!err) {
        result = ctx->eval_result;
        break;
      }
      if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
        return err;
      }
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_RETURN) {
      cubec_value_t err = cubec_eval_statement_return(ctx, node, filename);
      if (!err) {
        result = ctx->eval_result;
        break;
      }
      if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
        return err;
      }
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_FUNCTION) {
      cubec_value_t err = cubec_eval_statement_function(ctx, node, filename);
      if (!err) {
        result = ctx->eval_result;
        break;
      }
      if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
        return err;
      }
    } else {
      return cubec_context_create_compile_error(ctx, node, filename,
                                                "Unsupport top statement");
    }
  }
  cubec_scope_t current = ctx->current;
  ctx->current = parent;
  result =
      cubec_context_create_value(ctx, result->type, false, result->data, NULL);
  ctx->current = current;
  while (ctx->current != parent) {
    cubec_context_pop_scope(ctx);
  }
  return result;
}