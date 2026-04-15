#include "eval/literal_char.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/numeric.h"
#include "engine/value.h"
#include <stdbool.h>
#include <string.h>
value_t eval_literal_char(context_t ctx, ast_node_t node) {
  allocator_t allocator = context_get_allocator(ctx);
  char *s = location_get_str(node->loc, allocator);
  if (strlen(s) != 1) {
    allocator_free(allocator, s);
    return create_compile_error(ctx, node,
                                "Multi-character character constant");
  }
  char c = s[0];
  allocator_free(allocator, s);
  value_t value = create_u8(ctx, c, false, NULL);
  value_set_comptime(value, true);
  return value;
}