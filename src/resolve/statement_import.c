#include "resolve/statement_import.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/path.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/module.h"
#include "engine/value.h"

value_t resolve_statement_import(context_t ctx, ast_node_t node) {
  allocator_t allocator = context_get_allocator(ctx);
  ast_node_t source = ast_get_child(node, "source");
  ast_node_t identifier = ast_get_child(node, "identifier");
  char *src = location_get_str(source->loc, allocator);
  module_t mod = context_get_module(ctx);
  const char *dirname = module_get_dirname(mod);
  path_t path = create_path(allocator, dirname);
  path_t current = create_path(allocator, src);
  path_t fullpath = path_concat(path, allocator, current);
  allocator_free(allocator, path);
  allocator_free(allocator, current);
  allocator_free(allocator, src);
  char *filename = path_to_string(fullpath, allocator);
  allocator_free(allocator, fullpath);
  value_t value = context_load_module(ctx, filename);
  allocator_free(allocator, filename);
  if (value_is_error(value)) {
    value = convert_comptime_error(ctx, node, value);
    context_push_error(ctx, value);
    return context_get_undefined(ctx);
  }
  char *name = location_get(identifier->loc, allocator);
  value = value_clone(value, allocator);
  value_t err = context_declar(ctx, name, value);
  allocator_free(allocator, name);
  if (value_is_error(err)) {
    err = convert_comptime_error(ctx, node, err);
    context_push_error(ctx, err);
    return context_get_undefined(ctx);
  }
  return context_get_undefined(ctx);
}