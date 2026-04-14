#include "eval/literal_identifier.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
cubec_value_t cubec_eval_literal_identifier(cubec_context_t ctx,
                                            cubec_ast_node_t node) {
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  char *id = cubec_location_get(node->loc, allocator);
  cubec_value_t value = cubec_context_load(ctx, id);
  cubec_allocator_free(allocator, id);
  if (cubec_value_is_error(value)) {
    return cubec_convert_compile_error(ctx, node, value);
  }
  return value;
}