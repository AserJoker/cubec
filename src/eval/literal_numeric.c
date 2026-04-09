#include "eval/literal_numeric.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/numeric.h"
#include <stdbool.h>

cubec_value_t cubec_eval_literal_numeric(cubec_context_t ctx,
                                         cubec_ast_node_t node) {
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  const char *s = node->loc.begin.offset;
  bool is_floating = false;
  for (const char *c = s; c != node->loc.end.offset; c++) {
    if (*c == 'e' || *c == 'E' || *c == '.') {
      is_floating = true;
      break;
    }
  }
  if (is_floating) {
    double val = 0;
    cubec_cstring_to_dec(s, &val);
    return cubec_create_float64(ctx, val, true, NULL);
  } else {
    size_t val = 0;
    if (*s == '0' && (*(s + 1) == 'x' || *(s + 1) == 'X')) {
      cubec_cstring_to_int(s, &val, 16);
    } else if (*s == '0' && (*(s + 1) == 'o' || *(s + 1) == 'O')) {
      cubec_cstring_to_int(s, &val, 8);
    } else if (*s == '0' && (*(s + 1) == 'b' || *(s + 1) == 'b')) {
      cubec_cstring_to_int(s, &val, 2);
    } else {
      cubec_cstring_to_int(s, &val, 10);
    }
    return cubec_create_int32(ctx, val, true, NULL);
  }
}