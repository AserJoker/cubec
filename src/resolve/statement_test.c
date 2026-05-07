#include "resolve/statement_test.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/scope.h"
#include "engine/value.h"
#include "resolve/statement_block.h"
#include <stdio.h>

value_t resolve_statement_test(context_t ctx, ast_node_t node) {
  allocator_t allocator = context_get_allocator(ctx);
  ast_node_t name_node = ast_get_child(node, "name");
  ast_node_t body = ast_get_child(node, "body");
  char *name = location_get(name_node->loc, allocator);
  fprintf(stdout, "test %s start\n", name);
  bool is_comptime = context_set_comptime(ctx, true);

  scope_t current = context_get_scope(ctx);
  context_push_scope(ctx);
  scope_t scope = context_get_scope(ctx);
  value_t err = resolve_statement_block(ctx, body);
  err = value_clone(err, allocator);
  scope_store(current, allocator, NULL, err);
  context_set_scope(ctx, current);
  allocator_free(allocator, scope);

  context_set_comptime(ctx, is_comptime);
  if (value_is_error(err)) {
    fprintf(stdout, "%s\ntest %s failed\n", error_get_message(err), name);
  } else {
    fprintf(stdout, "test %s success\n", name);
  }
  allocator_free(allocator, name);
  return context_get_undefined(ctx);
}