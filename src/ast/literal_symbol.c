#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
#include <unicode/utypes.h>

static const char *symbols[] = {
    "[*]", ">>=", "<<=", "??=", "...", "==", "!=", ">=", "<=", "+=", "-=", "*=",
    "/=",  "%=",  "|=",  "&=",  "^=",  "??", "||", "&&", ">>", "<<", "+",  "-",
    "*",   "/",   "%",   "?",   ":",   ".",  ",",  "=",  ";",  "<",  ">",  "&",
    "|",   "!",   "^",   "~",   "(",   ")",  "{",  "}",  "[",  "]",  NULL,
};

cubec_ast_node_t cubec_read_ast_literal_symbol(cubec_allocator_t allocator,
                                               cubec_position_t *position,
                                               const char *end,
                                               const char *filename) {
  cubec_position_t current = *position;
  size_t offset = 0;
  for (;;) {
    if (!symbols[offset]) {
      return NULL;
    }
    current = *position;
    const char *src = symbols[offset];
    while (*src) {
      int32_t code = cubec_ast_read_code(&current, end, filename);
      if (code < 0) {
        return cubec_create_ast_error(allocator, *position, current, filename,
                                      "invalid unicode code");
      }
      if (*src != code) {
        break;
      }
      src++;
    }
    if (!*src) {
      break;
    }
    offset++;
  }
  cubec_ast_node_t symbol =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LITERAL_SYMBOL);
  symbol->loc.begin = *position;
  symbol->loc.end = current;
  symbol->loc.filename = filename;
  *position = current;
  return symbol;
}