#include "runtime/literal_char.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include <stdio.h>
cubec_value_t cubec_run_literal_char(cubec_context_t ctx, cubec_vm_t vm,
                                     cubec_ast_literal_char_t node) {
  char c = 0;
  const char *src = node->super.loc.begin.offset + 1;
  if (*src == '\\') {
    src++;
    if (*src == 'n') {
      c = '\n';
    } else if (*src == 'r') {
      c = '\r';
    } else if (*src == 't') {
      c = '\t';
    } else if (*src == 'b') {
      c = '\b';
    } else if (*src == 'f') {
      c = '\f';
    } else if (*src == 'a') {
      c = '\a';
    } else if (*src == '\\') {
      c = '\\';
    } else if (*src == '\'') {
      c = '\'';
    } else if (*src == '\"') {
      c = '\"';
    } else if (*src == '\?') {
      c = '\?';
    } else if (*src == '\0') {
      c = '\0';
    } else {
      char msg[128];
      char *s = cubec_location_get(node->super.loc, ctx->allocator);
      sprintf(msg, "Invalid charator %s", s);
      cubec_allocator_free(ctx->allocator, s);
      return cubec_context_create_error(ctx, msg, NULL);
    }
  } else {
    c = *src;
  }
  return cubec_context_create_int8(ctx, c, NULL);
}