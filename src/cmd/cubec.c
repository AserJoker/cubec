
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/program.h"
#include "astwriter/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/value.h"
#include <inttypes.h>
#include <stdio.h>
int main(int argc, char *argv[]) {
  cubec_allocator_t allocator = cubec_create_allocator(NULL);
  FILE *fp = fopen("./main.cubec", "rb");
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  char *source = cubec_allocator_alloc(allocator, len + 1, NULL);
  fseek(fp, 0, SEEK_SET);
  fread(source, len, 1, fp);
  source[len] = 0;
  fclose(fp);
  cubec_position_t begin;
  begin.column = 1;
  begin.line = 1;
  begin.offset = source;
  cubec_ast_node_t root =
      cubec_read_ast_program(allocator, &begin, source + len);
  if (root->type == CUBEC_NODE_TYPE_ERROR) {
    printf("Failed to compile: %s \n at %s:%" PRIuPTR ":%" PRIuPTR "\n",
           ((cubec_error_t)root)->message, "./main.cubec", root->loc.end.line,
           root->loc.end.column);
  } else {
    cubec_value_t ast = cubec_write_ast_node(root, allocator);
    char *json = cubec_value_to_json(ast, allocator);
    printf("%s\n", json);
    cubec_allocator_free(allocator, json);
    cubec_allocator_free(allocator, ast);
  }
  cubec_allocator_free(allocator, root);
  cubec_allocator_free(allocator, source);
  cubec_delete_allocator(allocator);
  return 0;
}