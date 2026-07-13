#include "cubec/statement.h"
#include "core/error.h"
#include "cubec/statement_block.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/statement_empty.h"
#include "cubec/statement_expression.h"
#include "cubec/statement_function.h"
#include "cubec/statement_interface.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_enum.h"
#include "cubec/statement_cunion.h"
#include "cubec/statement_union.h"
#include "cubec/statement_if.h"
#include "cubec/statement_while.h"
#include "cubec/statement_do_while.h"
#include "cubec/statement_for.h"
#include "cubec/statement_foreach.h"
#include "cubec/statement_import.h"
#include "cubec/statement_return.h"
#include "cubec/statement_switch.h"

node_t read_statement(allocator_t allocator, vec_t tokens, size_t *position,
                      const char *filename) {
  size_t current = *position;

  /* Try specific statement types first (they have distinguishing prefixes) */

  /* Try block statement ({...}) */
  node_t node = TRY_LOCAL(onerror, read_statement_block(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try declaration statement (var ...) */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_declaration(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try type declaration statement (type ...) */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_declaration_type(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try function statement (func ... / export func ... / inline func ... / extern func ...) */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_function(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try interface statement (interface ... / export interface ...) */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_interface(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try struct statement (struct ... / export struct ...) */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_struct(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try enum statement (enum ... / export enum ...) */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_enum(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try cunion statement (cunion ...) */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_cunion(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try union statement (union ... / export union ...) */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_union(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try if statement (if(...) { } else ...) */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_if(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try while statement (while(...) { }) */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_while(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try do-while statement (do { } while(...);) */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_do_while(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try for statement (for(init; cond; incr) { }) */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_for(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try foreach statement (foreach(name : iter) { }) */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_foreach(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try switch statement (switch(value) { case(...) -> { } else -> { } }) */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_switch(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try import statement (import ...) */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_import(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try return statement (return ...;) */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_return(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try empty statement (;) */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_empty(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Expression statement is the fallback — it has no distinguishing prefix,
     any expression can start it, so it must be tried last. */
  current = *position;
  node = TRY_LOCAL(onerror, read_statement_expression(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  return NULL;

onerror:
  return NULL;
}
