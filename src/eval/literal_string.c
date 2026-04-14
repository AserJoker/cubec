#include "eval/literal_string.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/str.h"
#include "engine/value.h"

cubec_value_t cubec_eval_literal_string(cubec_context_t ctx,
                                        cubec_ast_node_t node) {
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  char *s = cubec_location_get_str(node->loc, allocator);
  cubec_value_t str = cubec_create_str(ctx, s, NULL);
  cubec_allocator_free(allocator, s);
  return str;
}