#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "resolve/literal_identifier.h"

value_t resolve_literal_identifier(context_t ctx, ast_node_t node) {
  location_t loc = node_get_location(node);
  char *src = location_get(loc, ctx->allocator);
  value_t value = context_load(ctx, src);
  allocator_free(ctx->allocator, src);
  if (value->type->kind == TYPE_KIND_ERROR) {
    return convert_comptime_error(ctx, node_get_location(node), value);
  }
  return value;
}