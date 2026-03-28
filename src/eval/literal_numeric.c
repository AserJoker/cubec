#include "eval/literal_numeric.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/map.h"
#include "engine/context.h"
#include "engine/value.h"
#include <math.h>
#include <stdbool.h>

cubec_value_t cubec_eval_literal_numeric(cubec_context_t ctx,
                                         cubec_ast_node_t node,
                                         const char *filename) {
  cubec_value_t value = NULL;
  cubec_ast_node_t flag = cubec_map_get(node->children, "flag", NULL);
  char *s = cubec_location_get(node->loc, ctx->allocator);
  const char *pstr = s;
  if (*pstr == '0' && (*(pstr + 1) == 'x' || *(pstr + 1) == 'X')) {
    pstr += 2;
    uint64_t val = 0;
    while (*pstr) {
      if (*pstr >= '0' && *pstr <= '9') {
        val = val * 16 + *pstr - '0';
      } else if (*pstr >= 'a' && *pstr <= 'f') {
        val += val * 16 + *pstr - 'a' + 10;
      } else if (*pstr >= 'A' && *pstr <= 'F') {
        val += val * 16 + *pstr - 'A' + 10;
      } else if (*pstr != '_') {
        break;
      }
      pstr++;
    }
    value = cubec_context_create_int32(ctx, val, false, NULL);
  } else if (*pstr == '0' && (*(pstr + 1) == 'o' || *(pstr + 1) == 'O')) {
    pstr += 2;
    uint64_t val = 0;
    while (*pstr) {
      if (*pstr >= '0' && *pstr <= '7') {
        val = val * 8 + *pstr - '0';
      } else if (*pstr != '_') {
        break;
      }
      pstr++;
    }
    value = cubec_context_create_int32(ctx, val, false, NULL);
  } else if (*pstr == '0' && (*(pstr + 1) == 'b' || *(pstr + 1) == 'B')) {
    pstr += 2;
    uint64_t val = 0;
    while (*pstr) {
      if (*pstr >= '0' && *pstr <= '1') {
        val = val * 2 + *pstr - '0';
      } else if (*pstr != '_') {
        break;
      }
      pstr++;
    }
    if (*pstr == 'u') {
      if (*(pstr + 1) == 'l') {
        value = cubec_context_create_uint64(ctx, val, false, NULL);
      } else {
        value = cubec_context_create_uint32(ctx, val, false, NULL);
      }
    } else if (*pstr == 'l') {
      value = cubec_context_create_int64(ctx, val, false, NULL);
    } else {
      value = cubec_context_create_int32(ctx, val, false, NULL);
    }
  } else {
    uint64_t val = 0;
    while (*pstr) {
      if (*pstr >= '0' && *pstr <= '9') {
        val = val * 10 + *pstr - '0';
      } else if (*pstr != '_') {
        break;
      }
      pstr++;
    }
    if (*pstr == '.') {
      double fval = val;
      pstr++;
      double mask = 0.1;
      while (*pstr) {
        if (*pstr >= '0' && *pstr <= '9') {
          fval = fval + (*pstr - '0') * mask;
        } else if (*pstr != '_') {
          break;
        }
        mask *= 0.1;
        pstr++;
      }
      if (*pstr == 'e' || *pstr == 'E') {
        pstr++;
        double p = 0;
        while (*pstr) {
          if (*pstr >= '0' && *pstr <= '9') {
            p = p * 10 + *pstr - '0';
          } else if (*pstr != '_') {
            break;
          }
          pstr++;
        }
        fval = pow(fval, p);
      }
      if (*pstr == 'f') {
        value = cubec_context_create_float32(ctx, fval, false, NULL);
      } else {
        value = cubec_context_create_float64(ctx, fval, false, NULL);
      }
    } else if (*pstr == 'e' || *pstr == 'E') {
      double fval = val;
      if (*pstr == 'e' || *pstr == 'E') {
        pstr++;
        double p = 0;
        while (*pstr) {
          if (*pstr >= '0' && *pstr <= '9') {
            p = p * 10 + *pstr - '0';
          } else if (*pstr != '_') {
            break;
          }
          pstr++;
        }
        fval = pow(fval, p);
      }
      if (*pstr == 'f') {
        value = cubec_context_create_float32(ctx, fval, false, NULL);
      } else {
        value = cubec_context_create_float64(ctx, fval, false, NULL);
      }
    } else {
      if (*pstr == 'u') {
        if (*(pstr + 1) == 'l') {
          value = cubec_context_create_uint64(ctx, val, false, NULL);
        } else {
          value = cubec_context_create_uint32(ctx, val, false, NULL);
        }
      } else if (*pstr == 'l') {
        value = cubec_context_create_int64(ctx, val, false, NULL);
      } else {
        value = cubec_context_create_int32(ctx, val, false, NULL);
      }
    }
  }
  cubec_allocator_free(ctx->allocator, s);
  return value;
}