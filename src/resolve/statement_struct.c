#include "engine/struct.h"
#include "engine/error.h"
#include "resolve/statement_struct.h"
#include "resolve/struct_declarator.h"

value_t resolve_statement_struct(context_t ctx, ast_node_t node) {
  ast_node_t struct_node = ast_get_child(node, "struct");
  allocator_t allocator = context_get_allocator(ctx);
  ast_node_t identifier = ast_get_child(struct_node, "identifier");
  if (!identifier) {
    value_t err =
        create_comptime_error(ctx, struct_node, "top struct missing name");
    if (context_is_comptime(ctx)) {
      return err;
    } else {
      context_push_error(ctx, err);
    }
    return context_get_undefined(ctx);
  }
  value_t stru = resolve_struct_declarator(ctx, struct_node);
  if (value_is_error(stru)) {
    if (context_is_comptime(ctx)) {
      return stru;
    } else {
      context_push_error(ctx, stru);
    }
  } else {
    if (context_get_type(ctx) == CONTEXT_TYPE_STRUCT) {
      type_t global = context_get_global(ctx);
      char *name = location_get(identifier->loc, allocator);
      struct_type_add_attribute(global, allocator, name, stru);
      allocator_free(allocator, name);
    } else {
      char *name = location_get(identifier->loc, allocator);
      value_t err = context_declar(ctx, name, value_clone(stru, allocator));
      allocator_free(allocator, name);
      if (value_is_error(err)) {
        if (context_is_comptime(ctx)) {
          return err;
        } else {
          err = convert_comptime_error(ctx, struct_node, err);
          context_push_error(ctx, err);
          return context_get_undefined(ctx);
        }
      }
    }
  }
  return context_get_undefined(ctx);
}