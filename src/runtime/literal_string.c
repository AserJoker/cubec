#include "runtime/literal_string.h"
#include "engine/context.h"
cubec_value_t cubec_run_literal_string(cubec_context_t ctx, cubec_vm_t vm,
                                       cubec_ast_literal_string_t node) {
  char s[node->super.loc.end.offset - node->super.loc.begin.offset - 2];
  const char *src = node->super.loc.begin.offset + 1;
  char *dst = &s[0];
  while (src != node->super.loc.end.offset - 1) {
    if (*src == '\\') {
      src++;
      if (*src == 'n') {
        *dst++ = '\n';
      } else if (*src == 'r') {
        *dst++ = '\r';
      } else if (*src == 't') {
        *dst++ = '\t';
      } else if (*src == 'b') {
        *dst++ = '\b';
      } else if (*src == 'f') {
        *dst++ = '\f';
      } else if (*src == 'a') {
        *dst++ = '\a';
      } else if (*src == '0') {
        *dst++ = '\0';
      } else {
        *dst++ = *src;
      }
      src++;
    } else {
      *dst++ = *src++;
    }
  }
  *dst = 0;
  return cubec_context_create_str(ctx, s, NULL);
}