#include "eval/function_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/type.h"
#include <stdbool.h>

cubec_value_t cubec_eval_function_declarator(cubec_context_t ctx,
                                             cubec_ast_node_t node,
                                             const char *filename) {
  cubec_ast_node_t type_node =
      cubec_ast_get_child(ctx->allocator, node, "type");
  cubec_ast_node_t args_node =
      cubec_ast_get_child(ctx->allocator, node, "args");
  cubec_ast_node_t kind = cubec_ast_get_child(ctx->allocator, node, "kind");
  cubec_value_t type_value = cubec_eval_type(ctx, type_node, filename);
  if (!type_value) {
    return NULL;
  }
  if (type_value->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return type_value;
  }
  cubec_type_t type = *(cubec_type_t *)type_value->data;
  cubec_array_t arguments = cubec_create_array(ctx->allocator, NULL);
  bool variadic = false;
  for (size_t idx = 0; idx < cubec_ast_get_length(args_node); idx++) {
    cubec_ast_node_t arg_node = cubec_array_get(args_node->items, idx);
    if (arg_node->type == CUBEC_NODE_TYPE_FUNCTION_ARGUMENT) {
      cubec_ast_node_t arg_type =
          cubec_ast_get_child(ctx->allocator, arg_node, "type");
      cubec_value_t type_value = cubec_eval_type(ctx, arg_type, filename);
      if (!type_value) {
        cubec_allocator_free(ctx->allocator, arguments);
        return NULL;
      }
      if (type_value->type->kind == CUBEC_TYPE_KIND_ERROR) {
        cubec_allocator_free(ctx->allocator, arguments);
        return type_value;
      }
      cubec_type_t type = *(cubec_type_t *)type_value->data;
      cubec_array_push(arguments, type);
    } else if (arg_node->type == CUBEC_NODE_TYPE_LITERAL_SYMBOL &&
               cubec_location_is(arg_node->loc, "...")) {
      variadic = true;
    }
  }
  cubec_type_t func_type = cubec_context_create_function_type(
      ctx, type, cubec_array_get_size(arguments),
      cubec_array_get_data(arguments), variadic);
  cubec_allocator_free(ctx->allocator, arguments);
  cubec_ast_node_t identifier =
      cubec_ast_get_child(ctx->allocator, node, "identifier");
  char *name = NULL;
  if (identifier) {
    name = cubec_location_get(identifier->loc, ctx->allocator);
  }
  cubec_value_t value =
      cubec_context_create_function(ctx, func_type, node, false, name);
  cubec_allocator_free(ctx->allocator, name);
  return value;
}