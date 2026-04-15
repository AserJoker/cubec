#include "eval/literal_numeric.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/string.h"
#include "engine/error.h"
#include "engine/numeric.h"
#include "engine/value.h"
#include <stdbool.h>
value_t eval_literal_numeric(context_t ctx, ast_node_t node) {

  allocator_t allocator = context_get_allocator(ctx);
  ast_node_t value_node = ast_get_child(node, "value");
  ast_node_t type = ast_get_child(node, "type");
  char *str = location_get(value_node->loc, allocator);
  bool floating = false;
  for (const char *ch = str; *ch; ch++) {
    if (*ch == 'e' || *ch == 'E' || *ch == '.') {
      floating = true;
      break;
    }
  }
  value_t value = NULL;
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
    cstring_to_int(str, &val, radix);
    if (type) {
      if (location_is(type->loc, "i8")) {
        value = create_i8(ctx, val, false, NULL);
      } else if (location_is(type->loc, "i16")) {
        value = create_i16(ctx, val, false, NULL);
      } else if (location_is(type->loc, "i32")) {
        value = create_i32(ctx, val, false, NULL);
      } else if (location_is(type->loc, "i64")) {
        value = create_i64(ctx, val, false, NULL);
      } else if (location_is(type->loc, "u8")) {
        value = create_u8(ctx, val, false, NULL);
      } else if (location_is(type->loc, "u16")) {
        value = create_u16(ctx, val, false, NULL);
      } else if (location_is(type->loc, "u32")) {
        value = create_u32(ctx, val, false, NULL);
      } else if (location_is(type->loc, "u64")) {
        value = create_u64(ctx, val, false, NULL);
      } else if (location_is(type->loc, "f32")) {
        value = create_f32(ctx, val, false, NULL);
      } else if (location_is(type->loc, "f64")) {
        value = create_f64(ctx, val, false, NULL);
      } else {
        value = create_compile_error(ctx, node, "unsupport numeric type");
      }
    } else {
      value = create_i32(ctx, val, false, NULL);
    }
  } else {
    double val = 0;
    cstring_to_dec(str, &val);
    if (type) {
      if (location_is(type->loc, "f32")) {
        value = create_f32(ctx, val, false, NULL);
      } else if (location_is(type->loc, "f64")) {
        value = create_f64(ctx, val, false, NULL);
      } else {
        value = create_compile_error(ctx, node, "unsupport numeric type");
      }
    } else {
      value = create_f64(ctx, val, false, NULL);
    }
  }
  allocator_free(allocator, str);
  return value;
}