
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/fs.h"
#include "core/location.h"
#include "core/path.h"
#include "core/position.h"
#include "core/string.h"
#include "reader/token.h"
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
  char *source = fs_read_file(allocator, filename);

  if (source) {
    position_t pos = {
        .column = 1,
        .offset = source,
        .line = 1,
    };
    size_t len = strlen(source);
    token_stream_t stream =
        create_token_stream(allocator, &pos, source + len, filename);
    if (!stream) {
      fprintf(stderr,
              "failed to compile: invalid token  at\n  %s:%" PRIuPTR
              ":%" PRIuPTR "\n",
              filename, pos.line, pos.column);
    } else {
      FILE *fp = fopen("./build/tokens.txt", "w");
      token_t token = NULL;
      while ((token = token_stream_get(stream)) != NULL) {
        switch (token->type) {
        case TOKEN_TYPE_SPACE:
          fprintf(fp, "%-25s", "TOKEN_TYPE_SPACE");
          break;
        case TOKEN_TYPE_COMMENT:
          fprintf(fp, "%-25s", "TOKEN_TYPE_COMMENT");
          break;
        case TOKEN_TYPE_NUMERIC:
          fprintf(fp, "%-25s", "TOKEN_TYPE_NUMERIC");
          break;
        case TOKEN_TYPE_IDENTIFIER:
          fprintf(fp, "%-25s", "TOKEN_TYPE_IDENTIFIER");
          break;
        case TOKEN_TYPE_KEYWORD:
          fprintf(fp, "%-25s", "TOKEN_TYPE_KEYWORD");
          break;
        case TOKEN_TYPE_STRING:
          fprintf(fp, "%-25s", "TOKEN_TYPE_STRING");
          break;
        case TOKEN_TYPE_CHAR:
          fprintf(fp, "%-25s", "TOKEN_TYPE_CHAR");
          break;
        case TOKEN_TYPE_SYMBOL:
          fprintf(fp, "%-25s", "TOKEN_TYPE_SYMBOL");
          break;
        case TOKEN_TYPE_EOF:
          fprintf(fp, "%-25s", "TOKEN_TYPE_EOF");
          break;
        }
        stream->position++;
        fprintf(fp, "| ");
        char *str = location_get(token->loc, allocator);
        char *encode_str = encode_cstring(allocator, str);
        fprintf(fp, "%s", encode_str);
        allocator_free(allocator, encode_str);
        allocator_free(allocator, str);
        fprintf(fp, " |");
        fprintf(fp, "\n");
      }
      fclose(fp);
      allocator_free(allocator, stream);
    }
    ast_node_t root = read_ast_node(allocator, filename, source, NULL);
    if (root->type == NODE_TYPE_ERROR) {
      fprintf(stderr,
              "failed to compile:%s at\n  %s:%" PRIuPTR ":%" PRIuPTR "\n",
              root->error->message, root->error->filename,
              root->error->end.line + 1, root->error->end.column + 1);
    } else {
      printf("compile success\n");
    }
    allocator_free(allocator, root);
    allocator_free(allocator, source);
  } else {
    fprintf(stderr, "failed to compile: %s is not exist\n", filename);
  }
  allocator_free(allocator, filename);
  delete_allocator(allocator);
  return 0;
}