#include "cubec/node.h"
#include "core/token.h"
#include "cubec/token.h"

void skip_whitespace(vec_t tokens, size_t *position) {
  size_t current = *position;
  while (true) {
    token_t token = vec_get(tokens, current);
    cubec_token_kind_t kind = token_get_kind(token);
    if (kind == CUBEC_TOKEN_WHITESPACE || kind == CUBEC_TOKEN_COMMENT ||
        kind == CUBEC_TOKEN_MULTILINE_COMMENT ||
        kind == CUBEC_TOKEN_ERROR) {
      current++;
    } else {
      *position = current;
      return;
    }
  }
}