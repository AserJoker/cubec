#include "runtime/literal_numeric.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/string.h"
#include "engine/context.h"
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

cubec_value_t cubec_run_literal_numeric(cubec_context_t ctx, cubec_vm_t vm,
                                        cubec_ast_literal_numeric_t node) {
  const char *str = node->super.loc.begin.offset;
  if (node->is_float || node->is_exp) {
    double val = 0;
    str = cubec_cstring_to_dec(str, &val);
    if (*str == 'f') {
      return cubec_context_create_float32(ctx, (float)val, NULL);
    } else {
      return cubec_context_create_float64(ctx, val, NULL);
    }
  } else {
    size_t val = 0;
    if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X')) {
      str += 2;
      str = cubec_cstring_to_int(str, &val, 16);
    } else if (*str == '0' && (*(str + 1) == 'o' || *(str + 1) == 'O')) {
      str += 2;
      str = cubec_cstring_to_int(str, &val, 8);
    } else if (*str == '0' && (*(str + 1) == 'b' || *(str + 1) == 'B')) {
      str += 2;
      str = cubec_cstring_to_int(str, &val, 2);
    } else {
      str = cubec_cstring_to_int(str, &val, 10);
    }
    if (!node->flag) {
      if (val > INT_MAX) {
        char msg[256];
        sprintf(msg, "numeric %" PRIuPTR " is out of int32 range", val);
        return cubec_context_create_error(ctx, msg, NULL);
      }
      return cubec_context_create_int32(ctx, (int32_t)val, NULL);
    } else if (cubec_location_is(node->flag->loc, "u")) {
      if (val > UINT_MAX) {
        char msg[256];
        sprintf(msg, "numeric %" PRIuPTR " is out of uint32 range", val);
        return cubec_context_create_error(ctx, msg, NULL);
      }
      return cubec_context_create_uint32(ctx, (int32_t)val, NULL);
    } else if (cubec_location_is(node->flag->loc, "l")) {
      if (val > INT64_MAX) {
        char msg[256];
        sprintf(msg, "numeric %" PRIuPTR " is out of int64 range", val);
        return cubec_context_create_error(ctx, msg, NULL);
      }
      return cubec_context_create_uint32(ctx, (int32_t)val, NULL);
    } else if (cubec_location_is(node->flag->loc, "ul")) {
      return cubec_context_create_uint64(ctx, val, NULL);
    } else {
      char msg[256];
      char *flag = cubec_location_get(node->flag->loc, ctx->allocator);
      sprintf(msg, "Unsupport numeric flag %s", flag);
      cubec_allocator_free(ctx->allocator, flag);
      return cubec_context_create_error(ctx, msg, NULL);
    }
  }
}