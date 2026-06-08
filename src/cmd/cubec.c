
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
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  allocator_t allocator = create_allocator(NULL);
  context_t ctx = create_context(allocator);
  value_t value = context_load_module(ctx, "./demo/main.cubec");
  if (value->type->kind != TYPE_KIND_ERROR) {
    struct stat sts;
    if (stat("./demo/build", &sts)) {
      mkdir("./demo/build", 0777);
    }
    stream_t stream = context_write_c(ctx);
    string_t str = stream_get_string(stream);
    const char *src = string_get(str);
    FILE *fp = fopen("./demo/build/main.c", "w");
    fprintf(fp, "%s", src);
    fclose(fp);
    allocator_free(ctx->allocator, str);
    allocator_free(ctx->allocator, stream);
    allocator_free(allocator, ctx);
    delete_allocator(allocator);
    char *gcc_args[] = {"gcc", "./demo/build/main.c", "-o", "./demo/build/main",
                        NULL};
    execv("/opt/gcc/gcc-16.1.0/bin/gcc", gcc_args);
  } else {
    allocator_free(allocator, ctx);
    delete_allocator(allocator);
    return -1;
  }
  return 0;
}