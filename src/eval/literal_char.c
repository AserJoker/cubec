#include "eval/literal_char.h"
#include "core/allocator.h"
#include "engine/context.h"
cubec_value_t cubec_eval_literal_char(cubec_context_t ctx,
                                      cubec_ast_node_t node) {
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  return cubec_context_get_undefined(ctx);
}