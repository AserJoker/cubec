#include "resolve/literal_string.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/str.h"
value_t resolve_literal_string(context_t ctx, ast_node_t node) {
  location_t loc = node_get_location(node);
  char *src = location_get_str(loc, ctx->allocator);
  value_t value = create_comptime_str(ctx, src);
  allocator_free(ctx->allocator, src);
  return value;
}