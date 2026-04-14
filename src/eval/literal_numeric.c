#include "eval/literal_numeric.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/string.h"
#include "engine/error.h"
#include "engine/numeric.h"
#include "engine/value.h"
#include <stdbool.h>
cubec_value_t cubec_eval_literal_numeric(cubec_context_t ctx,
                                         cubec_ast_node_t node) {

  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  cubec_ast_node_t value_node = cubec_ast_get_child(node, "value");
  cubec_ast_node_t type = cubec_ast_get_child(node, "type");
  char *str = cubec_location_get(value_node->loc, allocator);
  bool floating = false;
  for (const char *ch = str; *ch; ch++) {
    if (*ch == 'e' || *ch == 'E' || *ch == '.') {
      floating = true;
      break;
    }
  }
  cubec_value_t value = NULL;
  if (!floating) {
    uint64_t val = 0;
    int radix = 10;
    const char *s = str;
    if (*s == '0' && *(s + 1) == 'x' && *(s + 1) == 'X') {
      s += 2;
      radix = 16;
    } else if (*s == '0' && *(s + 1) == 'o' && *(s + 1) == 'O') {
      s += 2;
      radix = 8;
    } else if (*s == '0' && *(s + 1) == 'b' && *(s + 1) == 'B') {
      s += 2;
      radix = 2;
    }
    cubec_cstring_to_int(str, &val, radix);
    if (type) {
      if (cubec_location_is(type->loc, "i8")) {
        value = cubec_create_i8(ctx, val, false, NULL);
      } else if (cubec_location_is(type->loc, "i16")) {
        value = cubec_create_i16(ctx, val, false, NULL);
      } else if (cubec_location_is(type->loc, "i32")) {
        value = cubec_create_i32(ctx, val, false, NULL);
      } else if (cubec_location_is(type->loc, "i64")) {
        value = cubec_create_i64(ctx, val, false, NULL);
      } else if (cubec_location_is(type->loc, "u8")) {
        value = cubec_create_u8(ctx, val, false, NULL);
      } else if (cubec_location_is(type->loc, "u16")) {
        value = cubec_create_u16(ctx, val, false, NULL);
      } else if (cubec_location_is(type->loc, "u32")) {
        value = cubec_create_u32(ctx, val, false, NULL);
      } else if (cubec_location_is(type->loc, "u64")) {
        value = cubec_create_u64(ctx, val, false, NULL);
      } else if (cubec_location_is(type->loc, "f32")) {
        value = cubec_create_f32(ctx, val, false, NULL);
      } else if (cubec_location_is(type->loc, "f64")) {
        value = cubec_create_f64(ctx, val, false, NULL);
      } else {
        value = cubec_create_compile_error(ctx, node, "unsupport numeric type");
      }
    } else {
      value = cubec_create_i32(ctx, val, false, NULL);
    }
  } else {
    double val = 0;
    cubec_cstring_to_dec(str, &val);
    if (type) {
      if (cubec_location_is(type->loc, "f32")) {
        value = cubec_create_f32(ctx, val, false, NULL);
      } else if (cubec_location_is(type->loc, "f64")) {
        value = cubec_create_f64(ctx, val, false, NULL);
      } else {
        value = cubec_create_compile_error(ctx, node, "unsupport numeric type");
      }
    } else {
      value = cubec_create_f64(ctx, val, false, NULL);
    }
  }
  cubec_allocator_free(allocator, str);
  return value;
}