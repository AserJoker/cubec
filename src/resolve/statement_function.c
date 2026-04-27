#include "resolve/statement_function.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/struct.h"
#include "engine/value.h"
#include "resolve/function_declaration.h"
value_t resolve_statement_function(context_t ctx, ast_node_t node) {
  ast_node_t function_node = ast_get_child(node, "function");
  allocator_t allocator = context_get_allocator(ctx);
  ast_node_t identifier = ast_get_child(function_node, "identifier");
  ast_node_t kind = ast_get_child(function_node, "kind");
  if (!identifier) {
    value_t err =
        create_comptime_error(ctx, function_node, "top function missing name");
    if (context_is_comptime(ctx)) {
      return err;
    } else {
      context_push_error(ctx, err);
    }
    return context_get_undefined(ctx);
  }
  value_t function = resolve_function_declarator(ctx, function_node);
  if (value_is_error(function)) {
    if (context_is_comptime(ctx)) {
      return function;
    } else {
      context_push_error(ctx, function);
    }
  } else {
    if (context_get_type(ctx) == CONTEXT_TYPE_STRUCT) {
      type_t global = context_get_global(ctx);
      char *name = location_get(identifier->loc, allocator);
      struct_type_add_attribute(global, allocator, name, function);
      allocator_free(allocator, name);
    } else {
      char *name = location_get(identifier->loc, allocator);
      value_t err = context_declar(ctx, name, value_clone(function, allocator));
      allocator_free(allocator, name);
      if (value_is_error(err)) {
        if (context_is_comptime(ctx)) {
          return err;
        } else {
          err = convert_comptime_error(ctx, function_node, err);
          context_push_error(ctx, err);
          return context_get_undefined(ctx);
        }
      }
    }
  }
  ast_node_bind_value(allocator, function_node, function);
  return context_get_undefined(ctx);
}