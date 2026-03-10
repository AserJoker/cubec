#include "core/allocator.h"
#include "engine/context.h"
#include "engine/type.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  cubec_allocator_t allocator = cubec_create_allocator(NULL);
  cubec_context_t ctx = cubec_create_context(allocator);
  cubec_value_t res =
      cubec_context_eval(ctx, "./main.cubec", NULL, CUBEC_EVAL_MODULE);
  if (res->type->kind == CUBEC_VALUE_TYPE_ERROR) {
    const char **msg = res->data;
    fprintf(stderr, "%s\n", *msg);
  }
  cubec_allocator_free(allocator, ctx);
  cubec_delete_allocator(allocator);
  return 0;
}