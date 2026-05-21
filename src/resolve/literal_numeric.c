#include "resolve/literal_numeric.h"
#include "ast/node.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/float.h"
#include "engine/integer.h"
#include "engine/unsigned.h"
#include <math.h>
#include <stdbool.h>
#include <string.h>

value_t resolve_literal_numeric(context_t ctx, ast_node_t node) {
  location_t loc = node_get_location(node);
  const char *begin = loc.begin.offset;
  const char *end = loc.end.offset;
  if ((*begin == '0' && *(begin + 1) == 'x' || *(begin + 1) == 'X') ||
      *begin == '0' && *(begin + 1) == 'o' || *(begin + 1) == 'O' ||
      *begin == '0' && *(begin + 1) == 'b' || *(begin + 1) == 'B') {
    uint64_t val = 0;
    if (*begin == '0' && *(begin + 1) == 'x' || *(begin + 1) == 'X') {
      begin += 2;
      while (begin != end) {
        if (*begin >= '0' && *begin <= '9') {
          val = val * 16 + (*begin - '0');
        } else if (*begin >= 'a' && *begin <= 'f') {
          val = val * 16 + (*begin - 'a');
        } else if (*begin >= 'A' && *begin <= 'F') {
          val = val * 16 + (*begin - 'A');
        } else {
          break;
        }
        begin++;
      }
    } else if (*begin == '0' && *(begin + 1) == 'o' || *(begin + 1) == 'O') {
      begin += 2;
      while (begin != end) {
        if (*begin >= '0' && *begin <= '7') {
          val = val * 8 + (*begin - '0');
        } else {
          break;
        }
        begin++;
      }
    } else if (*begin == '0' && *(begin + 1) == 'b' || *(begin + 1) == 'B') {
      begin += 2;
      while (begin != end) {
        if (*begin >= '0' && *begin <= '1') {
          val = val * 2 + (*begin - '0');
        } else {
          break;
        }
        begin++;
      }
    }
    if (strncmp(begin, "i8", 2) == 0) {
      return create_comptime_i8(ctx, val, false, NULL);
    } else if (strncmp(begin, "i16", 3) == 0) {
      return create_comptime_i16(ctx, val, false, NULL);
    } else if (strncmp(begin, "i32", 3) == 0) {
      return create_comptime_i32(ctx, val, false, NULL);
    } else if (strncmp(begin, "i64", 3) == 0) {
      return create_comptime_i64(ctx, val, false, NULL);
    } else if (strncmp(begin, "u8", 2) == 0) {
      return create_comptime_u8(ctx, val, false, NULL);
    } else if (strncmp(begin, "u16", 3) == 0) {
      return create_comptime_u16(ctx, val, false, NULL);
    } else if (strncmp(begin, "u32", 3) == 0) {
      return create_comptime_u32(ctx, val, false, NULL);
    } else if (strncmp(begin, "u64", 3) == 0) {
      return create_comptime_u64(ctx, val, false, NULL);
    } else {
      return create_comptime_i32(ctx, val, false, NULL);
    }
  } else {
    uint64_t ival = 0;
    while (*begin >= '0' && *begin <= '9') {
      ival = ival * 10 + (*begin - '0');
      begin++;
    }
    if (*begin == '.' || *begin == 'e' || *begin == 'E') {
      float64_t fval = ival;
      if (*begin == '.') {
        begin++;
        float64_t mask = 0.1;
        while (*begin >= '0' && *begin <= '9') {
          fval += (*begin - '0') * mask;
          begin++;
          mask *= 0.1;
        }
      }
      if (*begin == 'e' || *begin == 'E') {
        begin++;
        float64_t exp = 0;
        while (*begin >= '0' && *begin <= '9') {
          exp = exp * 10 + (*begin - '0');
          begin++;
        }
        fval = pow(fval, exp);
      }
      if (strncmp(begin, "f16", 3) == 0) {
        return create_comptime_f16(ctx, ival, false, NULL);
      } else if (strncmp(begin, "f32", 3) == 0) {
        return create_comptime_f32(ctx, ival, false, NULL);
      } else if (strncmp(begin, "f64", 3) == 0) {
        return create_comptime_f64(ctx, ival, false, NULL);
      } else {
        return create_comptime_f32(ctx, ival, false, NULL);
      }
    } else {
      if (strncmp(begin, "i8", 2) == 0) {
        return create_comptime_i8(ctx, ival, false, NULL);
      } else if (strncmp(begin, "i16", 3) == 0) {
        return create_comptime_i16(ctx, ival, false, NULL);
      } else if (strncmp(begin, "i32", 3) == 0) {
        return create_comptime_i32(ctx, ival, false, NULL);
      } else if (strncmp(begin, "i64", 3) == 0) {
        return create_comptime_i64(ctx, ival, false, NULL);
      } else if (strncmp(begin, "u8", 2) == 0) {
        return create_comptime_u8(ctx, ival, false, NULL);
      } else if (strncmp(begin, "u16", 3) == 0) {
        return create_comptime_u16(ctx, ival, false, NULL);
      } else if (strncmp(begin, "u32", 3) == 0) {
        return create_comptime_u32(ctx, ival, false, NULL);
      } else if (strncmp(begin, "u64", 3) == 0) {
        return create_comptime_u64(ctx, ival, false, NULL);
      } else if (strncmp(begin, "f16", 3) == 0) {
        return create_comptime_f16(ctx, ival, false, NULL);
      } else if (strncmp(begin, "f32", 3) == 0) {
        return create_comptime_f32(ctx, ival, false, NULL);
      } else if (strncmp(begin, "f64", 3) == 0) {
        return create_comptime_f64(ctx, ival, false, NULL);
      } else {
        return create_comptime_i32(ctx, ival, false, NULL);
      }
    }
  }
}