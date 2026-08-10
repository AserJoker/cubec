#include "core/token_writer.h"
#include "core/string.h"

string_t token_writer_render(allocator_t allocator, vec_t tokens) {
  string_t str = allocator_create(allocator, &g_string_class, NULL);
  for (size_t i = 0; i < vec_get_size(tokens); i++) {
    token_t tok = vec_get(tokens, i);
    const char *text = token_get_string(tok);
    size_t length = token_get_string_length(tok);
    string_nconcat(str, text, length);
  }
  return str;
}
