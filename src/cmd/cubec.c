#include "core/allocator.h"
#include "engine/context.h"
#include <inttypes.h>
#include <string.h>

int main(int argc, char *argv[]) {
  cubec_allocator_t allocator = cubec_create_allocator(NULL);
  cubec_context_t ctx = cubec_create_context(allocator);
  cubec_context_eval(ctx, "./main.cubec", NULL);
  cubec_allocator_free(allocator, ctx);
  cubec_delete_allocator(allocator);
  return 0;
}