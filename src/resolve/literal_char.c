#include "resolve/literal_char.h"
#include "ast/node.h"
#include "engine/error.h"
#include "engine/unsigned.h"
#include <stdbool.h>
value_t resolve_literal_char(context_t ctx, ast_node_t node) {
  const char *begin = node->start->loc.begin.offset + 1;
  const char *end = node->start->loc.end.offset - 1;
  const char *pstr = begin;
  size_t code = 0;
  if (*pstr == '\\') {
    pstr++;
    if (*pstr == 'n') {
      code = '\n';
      pstr++;
    } else if (*pstr == 'r') {
      code = '\r';
      pstr++;
    } else if (*pstr == 't') {
      code = '\t';
      pstr++;
    } else if (*pstr == 'v') {
      code = '\v';
      pstr++;
    } else if (*pstr == 'a') {
      code = '\a';
      pstr++;
    } else if (*pstr == 'b') {
      code = '\b';
      pstr++;
    } else if (*pstr == '\\') {
      code = '\\';
      pstr++;
    } else if (*pstr == '\'') {
      code = '\'';
      pstr++;
    } else if (*pstr == '\"') {
      code = '\"';
      pstr++;
    } else if (*pstr == 'x' || *pstr == 'X') {
      pstr++;
      if (pstr + 2 != end) {
        return create_comptime_error(ctx, node_get_location(node),
                                     "invalid charactor");
      }
      code += *pstr;
      pstr++;
      code += *pstr;
      pstr++;
    } else {
      return create_comptime_error(ctx, node_get_location(node),
                                   "invalid charactor");
    }
  } else {
    code = *pstr;
    pstr++;
  }
  if (pstr != end || code > 127) {
    return create_comptime_error(ctx, node_get_location(node),
                                 "invalid charactor");
  }
  return create_comptime_u8(ctx, code, false, NULL);
}
