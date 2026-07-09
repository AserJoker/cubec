#include "cubec/statement.h"
#include "cubec/statement_block.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_empty.h"
#include "cubec/statement_expression.h"

node_t read_statement(allocator_t allocator, vec_t tokens, size_t *position,
                      const char *filename) {
  size_t current = *position;

  /* Try specific statement types first (they have distinguishing prefixes) */

  /* Try block statement ({...}) */
  node_t node = read_statement_block(allocator, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try declaration statement (var ...) */
  current = *position;
  node = read_statement_declaration(allocator, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try empty statement (;) */
  current = *position;
  node = read_statement_empty(allocator, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Expression statement is the fallback — it has no distinguishing prefix,
     any expression can start it, so it must be tried last. */
  current = *position;
  node = read_statement_expression(allocator, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  return NULL;
}
