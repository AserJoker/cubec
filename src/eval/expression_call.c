#include "eval/expression_call.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/array.h"
#include "core/map.h"
#include "engine/context.h"
#include "engine/function.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"

cubec_value_t cubec_eval_expression_call(cubec_context_t ctx,
                                         cubec_ast_node_t expr,
                                         const char *filename) {
  cubec_ast_node_t callee_node = cubec_map_get(expr->children, "callee", NULL);
  if (callee_node->type == CUBEC_NODE_TYPE_EXPRESSION_MEMBER) {
    return ctx->value_undefined;
  } else {
    cubec_function_meta_t meta = NULL;
    cubec_value_t callee = cubec_eval_expression(ctx, callee_node, filename);
    if (!callee) {
      return NULL;
    }
    if (callee->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return callee;
    }
    if (callee->type->kind == CUBEC_TYPE_KIND_FUNCTION) {
      meta = callee->type->meta;
    }
    cubec_ast_node_t args_node = cubec_map_get(expr->children, "args", NULL);
    size_t argc = cubec_array_get_size(args_node->items);
    cubec_value_t argv[argc];
    size_t idx = 0;
    for (size_t idx = 0; idx < cubec_array_get_size(args_node->items); idx++) {
      cubec_ast_node_t arg_node = cubec_array_get(args_node->items, idx);
      cubec_value_t arg = cubec_eval_expression(ctx, arg_node, filename);
      if (!arg) {
        return NULL;
      }
      if (arg->type->kind == CUBEC_TYPE_KIND_ERROR) {
        return arg;
      }
      if (meta && idx < cubec_array_get_size(meta->args)) {
        cubec_type_t type = cubec_array_get(meta->args, idx);
        argv[idx] = cubec_context_convert(ctx, type, arg);
      } else {
        argv[idx] = arg;
      }
    }
    cubec_value_t res = cubec_context_call(ctx, callee, argc, argv);
    if (res->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return cubec_context_create_compile_error(ctx, expr, filename,
                                                *(const char **)res->data);
    }
    return res;
  }
}