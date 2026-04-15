#include "eval/literal_identifier.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
value_t eval_literal_identifier(context_t ctx, ast_node_t node) {
  allocator_t allocator = context_get_allocator(ctx);
  char *id = location_get(node->loc, allocator);
  value_t value = context_load(ctx, id);
  allocator_free(allocator, id);
  if (value_is_error(value)) {
    return convert_compile_error(ctx, node, value);
  }
  return value;
}