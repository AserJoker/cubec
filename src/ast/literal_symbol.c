#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#include <unicode/utypes.h>
static void cubec_ast_literal_symbol_dispose(cubec_ast_literal_symbol_t self,
                                             cubec_allocator_t allocator) {
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_literal_symbol_t
cubec_create_ast_literal_symbol(cubec_allocator_t allocator) {
  cubec_ast_literal_symbol_t symbol = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_literal_symbol_t),
      (cubec_dispose_fn_t)cubec_ast_literal_symbol_dispose);
  cubec_ast_node_initialize(allocator, &symbol->super);
  symbol->super.type = CUBEC_NODE_TYPE_LITERAL_SYMBOL;
  return symbol;
}

static const char *symbols[] = {
    ">>=", "<<=", "??=", "==", "!=", ">=", "<=", "+=", "-=", "*=", "/=", "%=",
    "|=",  "&=",  "^=",  "??", "||", "&&", ">>", "<<", "++", "--", "+",  "-",
    "*",   "/",   "%",   "?",  ":",  ".",  ",",  "=",  ";",  "<",  ">",  "&",
    "|",   "!",   "^",   "~",  "(",  ")",  "{",  "}",  "[",  "]",  NULL,
};

cubec_ast_node_t cubec_read_ast_literal_symbol(cubec_allocator_t allocator,
                                               cubec_position_t *position,
                                               const char *end) {
  cubec_position_t current = *position;
  size_t offset = 0;
  for (;;) {
    if (!symbols[offset]) {
      return NULL;
    }
    current = *position;
    const char *src = symbols[offset];
    while (*src) {
      int32_t code = cubec_ast_read_code(&current, end);
      if (code < 0) {
        return cubec_create_ast_error(allocator, *position, current,
                                      "Invalid unicode code");
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
  cubec_ast_literal_symbol_t symbol =
      cubec_create_ast_literal_symbol(allocator);
  symbol->super.loc.begin = *position;
  symbol->super.loc.end = current;
  *position = current;
  return &symbol->super;
}