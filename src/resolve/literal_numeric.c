#include "resolve/literal_numeric.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/float.h"
#include "engine/integer.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

value_t resolve_literal_numeric(context_t ctx, ast_node_t node) {
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t value = ast_get_child(node, "value");
  allocator_t allocator = context_get_allocator(ctx);
  char *num = location_get(node->loc, allocator);
  bool floating = false;
  for (size_t idx = 0; num[idx]; idx++) {
    if (num[idx] == 'e' || num[idx] == 'E' || num[idx] == '.') {
      floating = true;
      break;
    }
  }
  value_t result = NULL;
  if (floating) {
    double val = 0;
    cstring_to_dec(num, &val);
    if (type) {
      if (location_is(type->loc, "f16")) {
        result = create_comptime_f16(ctx, val, false, NULL);
      } else if (location_is(type->loc, "f32")) {
        result = create_comptime_f32(ctx, val, false, NULL);
      } else if (location_is(type->loc, "f64")) {
        result = create_comptime_f64(ctx, val, false, NULL);
      } else {
        result = create_error(ctx, "unsupport numeric type");
      }
    } else {
      result = create_comptime_f64(ctx, val, false, NULL);
    }
  } else {
    uint64_t val = 0;
    int radix = 10;
    if (*num == '0' && (*(num + 1) == 'x' || *(num + 1) == 'X')) {
      cstring_to_int(num + 2, &val, 16);
    } else if (*num == '0' && (*(num + 1) == 'o' || *(num + 1) == 'O')) {
      cstring_to_int(num + 2, &val, 8);
    } else if (*num == '0' && (*(num + 1) == 'b' || *(num + 1) == 'B')) {
      cstring_to_int(num + 2, &val, 2);
    } else {
      cstring_to_int(num, &val, 10);
    }
    if (type) {
      if (location_is(type->loc, "i8")) {
        result = create_comptime_i8(ctx, val, false, NULL);
      } else if (location_is(type->loc, "i16")) {
        result = create_comptime_i16(ctx, val, false, NULL);
      } else if (location_is(type->loc, "i32")) {
        result = create_comptime_i32(ctx, val, false, NULL);
      } else if (location_is(type->loc, "i64")) {
        result = create_comptime_i64(ctx, val, false, NULL);
      } else if (location_is(type->loc, "u8")) {
        result = create_comptime_u8(ctx, val, false, NULL);
      } else if (location_is(type->loc, "u16")) {
        result = create_comptime_u16(ctx, val, false, NULL);
      } else if (location_is(type->loc, "u32")) {
        result = create_comptime_u32(ctx, val, false, NULL);
      } else if (location_is(type->loc, "u64")) {
        result = create_comptime_u64(ctx, val, false, NULL);
      } else {
        result = create_error(ctx, "unsupport numeric type");
      }
    } else {
      result = create_comptime_i32(ctx, val, false, NULL);
    }
  }
  allocator_free(allocator, num);
  if (value_is_error(result)) {
    result = convert_comptime_error(ctx, node, result);
  }
  return result;
}