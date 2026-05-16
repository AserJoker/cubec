
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/path.h"
#include "core/position.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
  char *filename = absolute(allocator, "./build/main.cubec");
  ast_doc_t doc = read_ast_node(allocator, filename, NULL);
  ast_node_t root = doc->node;
  if (root->type == NODE_TYPE_ERROR) {
    fprintf(stderr, "failed to compile:%s at\n  %s:%" PRIuPTR ":%" PRIuPTR "\n",
            root->error->message, root->error->filename,
            root->error->end.line + 1, root->error->end.column + 1);
  } else {
    printf("compile success\n");
  }
  allocator_free(allocator, doc);
  allocator_free(allocator, filename);
  delete_allocator(allocator);
  return 0;
}