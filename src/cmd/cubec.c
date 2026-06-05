
#include "ast/node.h"
#include "core/allocator.h"
#include "core/stream.h"
#include "core/string.h"
#include "engine/context.h"
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
  if (value->type->kind != TYPE_KIND_ERROR) {
    stream_t stream = context_write_c(ctx);
    string_t str = stream_get_string(stream);
    const char *src = string_get(str);
    FILE *fp = fopen("./build/main.c", "w");
    fprintf(fp, "%s", src);
    fclose(fp);
    allocator_free(ctx->allocator, str);
    allocator_free(ctx->allocator, stream);
  }
  allocator_free(allocator, ctx);
  delete_allocator(allocator);
  return 0;
}