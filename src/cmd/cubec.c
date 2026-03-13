#include "core/allocator.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

cubec_value_t print(cubec_context_t ctx, size_t argc, cubec_value_t *argv) {
  for (size_t idx = 0; idx < argc; idx++) {
    if (idx != 0) {
      printf(", ");
    }
    cubec_value_t val = argv[idx];
    val = cubec_context_to_str(ctx, val);
    if (val->type->kind == CUBEC_VALUE_TYPE_ERROR) {
      return val;
    }
    const char **msg = val->data;
    printf("%s:%s", argv[idx]->type->name, *msg);
  }
  printf("\n");
  return cubec_context_get_undefined(ctx);
}

int main(int argc, char *argv[]) {
  cubec_allocator_t allocator = cubec_create_allocator(NULL);
  cubec_context_t ctx = cubec_create_context(allocator);
  cubec_type_t print_fn_type = cubec_context_create_function_type(
      ctx, 0, NULL, ctx->named_types.void_type, true);
  cubec_value_t print_fn =
      cubec_context_create_native_function(ctx, print_fn_type, print, "print");
  cubec_value_t res =
      cubec_context_eval(ctx, "./main.cubec", NULL, CUBEC_EVAL_MODULE);
  if (res->type->kind == CUBEC_VALUE_TYPE_ERROR) {
    const char **msg = res->data;
    fprintf(stderr, "error: %s\n", *msg);
  }
  cubec_allocator_free(allocator, ctx);
  cubec_delete_allocator(allocator);
  return 0;
}