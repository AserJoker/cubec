
#include "ast/node.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  allocator_t allocator = create_allocator(NULL);
  context_t ctx = create_context(allocator);
  value_t value = context_load_module(ctx, "./build/main.cubec");
  if (value->type->kind == TYPE_KIND_ERROR) {
    char *msg = error_format(allocator, value);
    fprintf(stderr, "%s\n", msg);
    allocator_free(allocator, msg);
  }
  allocator_free(allocator, ctx);
  delete_allocator(allocator);
  return 0;
}