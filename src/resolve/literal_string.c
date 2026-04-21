#include "resolve/literal_string.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/str.h"
value_t resolve_literal_string(context_t ctx, ast_node_t node) {
  allocator_t allocator = context_get_allocator(ctx);
  char *str = location_get_str(node->loc, allocator);
  value_t value = create_str(ctx, str);
  allocator_free(allocator, str);
  return value;
}