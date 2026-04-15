#include "eval/literal_string.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/str.h"
#include "engine/value.h"
#include <stdio.h>

value_t eval_literal_string(context_t ctx, ast_node_t node) {
  allocator_t allocator = context_get_allocator(ctx);
  char *s = location_get_str(node->loc, allocator);
  value_t str = create_str(ctx, s, NULL);
  allocator_free(allocator, s);
  value_set_comptime(str, true);
  return str;
}