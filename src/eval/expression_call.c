#include "eval/expression_call.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/list.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"

cubec_value_t cubec_eval_expression_call(cubec_context_t ctx,
                                         cubec_ast_expression_call_t expr,
                                         const char *filename) {
  if (expr->callee->type == CUBEC_NODE_TYPE_EXPRESSION_MEMBER) {
    return ctx->value_undefined;
  } else {
    if (expr->callee->type == CUBEC_NODE_TYPE_LITERAL_IDENTIFIER) {
      if (cubec_location_is(expr->callee->loc, "typeof")) {
      } else if (cubec_location_is(expr->callee->loc, "sizeof")) {
      }
    }
    cubec_value_t callee = cubec_eval_expression(ctx, expr->callee, filename);
    if (callee->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return callee;
    }
    cubec_ast_list_node_t args = (cubec_ast_list_node_t)expr->args;
    size_t argc = cubec_list_get_size(args->items);
    cubec_value_t argv[argc];
    size_t idx = 0;
    for (cubec_list_node_t it = cubec_list_get_first(args->items);
         it != cubec_list_get_end(args->items); it = cubec_list_node_next(it)) {
      cubec_ast_node_t arg_node = cubec_list_node_get(it);
      cubec_value_t arg = cubec_eval_expression(ctx, arg_node, filename);
      if (arg->type->kind == CUBEC_TYPE_KIND_ERROR) {
        return arg;
      }
      argv[idx++] = arg;
    }
    cubec_value_t res = cubec_context_call(ctx, callee, argc, argv);
    if (res->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return cubec_context_create_compile_error(ctx, &expr->super, filename,
                                                *(const char **)res->data);
    }
    return res;
  }
}