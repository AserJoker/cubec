#include "ast/node.h"
#include "ast/program.h"
#include "core/allocator.h"
#include "core/path.h"
#include "core/position.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/module.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

char *absolute(cubec_allocator_t allocator, const char *filename) {
  cubec_path_t path = cubec_create_path(allocator, filename);
  cubec_path_t abs = cubec_path_absolute(path, allocator);
  cubec_allocator_free(allocator, path);
  char *fullname = cubec_path_to_string(abs, allocator);
  cubec_allocator_free(allocator, abs);
  return fullname;
}

char *read(cubec_allocator_t allocator, const char *fullname) {
  FILE *fp = fopen(fullname, "rb");
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char *source = cubec_allocator_alloc(allocator, len + 1, NULL);
  fread(source, len, 1, fp);
  source[len] = 0;
  fclose(fp);
  return source;
}

cubec_ast_node_t compile(cubec_allocator_t allocator, const char *filename,
                         const char *source) {
  cubec_position_t position = {
      .column = 1,
      .line = 1,
      .offset = source,
  };

  return cubec_read_ast_program(allocator, &position, source + strlen(source));
}

int main(int argc, char *argv[]) {
  cubec_allocator_t allocator = cubec_create_allocator(NULL);
  cubec_context_t ctx = cubec_create_context(allocator);
  char *filename = absolute(allocator, "./main.cubec");
  cubec_value_t err = cubec_context_load_module(ctx, filename);
  if (err->type == ctx->type_error) {
    const char *message = *(const char **)err->data;
    fprintf(stderr, "%s\n", message);
  } else {
    cubec_module_t mod = cubec_context_get_module(ctx, filename);
    const char *dst = cubec_string_get(mod->data);
    printf("%s\n", dst);
  }
  cubec_allocator_free(allocator, ctx);
  cubec_allocator_free(allocator, filename);
  cubec_delete_allocator(allocator);
  return 0;
}