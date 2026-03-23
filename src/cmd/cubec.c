#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/program.h"
#include "c/program.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/path.h"
#include "core/position.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/type.h"
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
  char *filename = absolute(allocator, "./main.cubec");
  char *source = read(allocator, filename);
  cubec_ast_node_t node = compile(allocator, filename, source);
  cubec_context_t ctx = cubec_create_context(allocator);
  if (node->type == CUBEC_NODE_TYPE_ERROR) {
    cubec_ast_error_t error = (cubec_ast_error_t)node;
    fprintf(stderr,
            "Failed to compile: %s at\n  %s:%" PRIuPTR ":%" PRIuPTR "\n",
            error->message, "./main.cubec", node->loc.end.line,
            node->loc.end.column);
  } else {
    cubec_string_t csource = cubec_create_string(allocator, NULL);
    cubec_value_t err =
        cubec_c_write_program(ctx, (cubec_ast_program_t)node, &csource);
    if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
      const char *error = *(const char **)err->data;
      fprintf(stderr, "%s\n", error);
    }
    cubec_allocator_free(allocator, csource);
  }
  cubec_allocator_free(allocator, ctx);
  cubec_allocator_free(allocator, node);
  cubec_allocator_free(allocator, source);
  cubec_allocator_free(allocator, filename);
  cubec_delete_allocator(allocator);
  return 0;
}