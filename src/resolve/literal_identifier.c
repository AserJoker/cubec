#include "resolve/literal_identifier.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
value_t resolve_literal_identifier(context_t ctx, ast_node_t node) {
  allocator_t allocator = context_get_allocator(ctx);
  char *name = location_get(node->loc, allocator);
  value_t value = context_load(ctx, name);
  allocator_free(allocator, name);
  if (value_is_error(value)) {
    value = convert_comptime_error(ctx, node, value);
  }
  if (context_is_comptime(ctx) && !value_is_comptime(value)) {
    return create_comptime_error(ctx, node, "value is not comptime");
  }
  return value;
}