
#include "core/allocator.h"
#include "core/error.h"
#include "core/location.h"
#include "core/token.h"
#include "core/vec.h"
#include "cubec/token.h"
#include <inttypes.h>
#include <stdio.h>

int _main(int argc, char *argv[]) {
  allocator_t allocator = create_allocator(NULL, NULL);
  FILE *fp = fopen("./demo/index.cubec", "rb");
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char source[len + 1];
  fread(source, len, 1, fp);
  source[len] = 0;
  vec_t tokens = NULL;
  tokens = TRY_LOCAL(
      onerror, resolve_token_list(allocator, "./demo/index.cubec", source));
  for (size_t idx = 0; idx < vec_get_size(tokens); idx++) {
    token_t token = vec_get(tokens, idx);
    cubec_token_kind_t kind = token_get_kind(token);
    location_t *location = token_get_location(token);
    switch (kind) {
    case CUBEC_TOKEN_WHITESPACE:
      printf("whitespace\n");
      break;
    case CUBEC_TOKEN_EOF:
      printf("eof\n");
      break;
    case CUBEC_TOKEN_COMMENT:
      printf("comment\n");
      break;
    case CUBEC_TOKEN_MULTILINE_COMMENT:
      printf("multiline_comment\n");
      break;
    case CUBEC_TOKEN_IDENTIFIER:
      printf("identifier\n");
      break;
    case CUBEC_TOKEN_NUMERIC:
      printf("numeric\n");
      break;
    case CUBEC_TOKEN_SYMBOL:
      printf("symbol\n");
      break;
    case CUBEC_TOKEN_KEYWORD:
      printf("keyword\n");
      break;
    case CUBEC_TOKEN_STRING:
      printf("string\n");
      break;
    }
  }
  allocator_free(allocator, tokens);
  delete_allocator(allocator);
  return 0;
onerror:
  allocator_free(allocator, tokens);
  delete_allocator(allocator);
  return -1;
}

int main(int argc, char *argv[]) {
  int res = _main(argc, argv);
  if (g_error) {
    char *err = error_to_string(g_error, NULL);
    fprintf(stderr, "%s", err);
    free(err);
    error_clear();
  }
  return res;
}