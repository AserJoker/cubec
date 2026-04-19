#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/program.h"
#include "core/allocator.h"
#include "core/path.h"
#include "core/position.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

char *absolute(allocator_t allocator, const char *name) {
  path_t path = create_path(allocator, name);
  path_t fullpath = path_absolute(path, allocator);
  char *result = path_to_string(fullpath, allocator);
  allocator_free(allocator, path);
  allocator_free(allocator, fullpath);
  return result;
}

int main(int argc, char *argv[]) {
  allocator_t allocator = create_allocator(NULL);

  char *filename = absolute(allocator, "./main.cubec");
  FILE *fp = fopen(filename, "rb");
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  char buf[len + 1];
  fseek(fp, 0, SEEK_SET);
  fread(buf, len, 1, fp);
  fclose(fp);
  buf[len] = 0;
  position_t pos = {
      .offset = buf,
      .line = 0,
      .column = 0,
  };
  ast_node_t program = read_ast_program(allocator, &pos, buf + len, filename);
  if (program->type == NODE_TYPE_ERROR) {
    ast_error_t err = (ast_error_t)program;
    fprintf(stderr, "Failed to compile: %s, at \n  %s:%" PRIdPTR ":%" PRIdPTR,
            err->message, filename, program->loc.end.line + 1,
            program->loc.end.column + 1);
  } else {
    char *json = ast_write_json(allocator, program);
    fp = fopen("main.json", "w");
    fprintf(fp, "%s", json);
    fclose(fp);
    allocator_free(allocator, json);
  }
  allocator_free(allocator, program);
  allocator_free(allocator, filename);
  delete_allocator(allocator);
  return 0;
}