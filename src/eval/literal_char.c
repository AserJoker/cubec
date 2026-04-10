#include "eval/literal_char.h"
#include "engine/error.h"
#include "engine/numeric.h"
#include <stdbool.h>
#include <stdio.h>

cubec_value_t cubec_eval_literal_char(cubec_context_t ctx,
                                      cubec_ast_node_t node) {
  const char *str = node->loc.begin.offset;
  str++;
  uint8_t c = 0;
  if (*str == '\\') {
    str++;
    if (*str == 'x') {
      str++;
      uint32_t val = 0;
      while ((*str >= '0' && *str <= '9') || *str >= 'a' && *str <= 'f' ||
             *str >= 'A' && *str <= 'F') {
        val *= 16;
        if (*str >= '0' && *str <= '9') {
          val += *str - '0';
        } else if (*str >= 'a' && *str <= 'f') {
          val += *str - 'a' + 10;
        } else if (*str >= 'A' && *str <= 'F') {
          val += *str - 'A' + 10;
        }
        str++;
      }
      if (val >= 256) {
        return cubec_create_compile_error(ctx, node,
                                          "Hex escape sequence out of range");
      }
      c = val;
    } else if (*str >= '0' && *str <= '7') {
      uint32_t val = 0;
      while ((*str >= '0' && *str <= '7')) {
        val *= 8;
        if (*str >= '0' && *str <= '7') {
          val += *str - '0';
        }
        str++;
      }
      if (val >= 256) {
        return cubec_create_compile_error(ctx, node,
                                          "Hex escape sequence out of range");
      }
      c = val;
    } else if (*str == 'u' || *str == 'U') {
      return cubec_create_compile_error(ctx, node,
                                        "Incomplete universal character name");
    } else {
      c = *str;
      str++;
    }
  } else {
    c = *str;
    str++;
  }
  if (*str != '\'') {
    return cubec_create_compile_error(ctx, node,
                                      "Multi-character character constant");
  }
  return cubec_create_u8(ctx, c, false, NULL);
}