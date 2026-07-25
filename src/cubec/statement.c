#include "cubec/statement.h"
#include "cubec/statement_block.h"
#include "cubec/statement_comptime.h"
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
#include "cubec/statement_break.h"
#include "cubec/statement_continue.h"
#include "cubec/statement_defer.h"
#include "engine/context.h"

node_t read_statement(context_t ctx, vec_t tokens, size_t *position,
                      const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;

  /* Try specific statement types first (they have distinguishing prefixes) */

  /* Try block statement ({...}) */
  node_t node = read_statement_block(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try comptime statement (comptime { } / comptime if / comptime for) —
     must be before declaration/function since 'comptime' is also a modifier */
  current = *position;
  node = read_statement_comptime(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try declaration statement (var ...) */
  current = *position;
  node = read_statement_declaration(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try type declaration statement (type ...) */
  current = *position;
  node = read_statement_declaration_type(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try function statement (func ... / export func ... / inline func ... / extern func ...) */
  current = *position;
  node = read_statement_function(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try interface statement (interface ... / export interface ...) */
  current = *position;
  node = read_statement_interface(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try struct statement (struct ... / export struct ...) */
  current = *position;
  node = read_statement_struct(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try enum statement (enum ... / export enum ...) */
  current = *position;
  node = read_statement_enum(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try cunion statement (cunion ...) */
  current = *position;
  node = read_statement_cunion(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try union statement (union ... / export union ...) */
  current = *position;
  node = read_statement_union(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try if statement (if(...) { } else ...) */
  current = *position;
  node = read_statement_if(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try while statement (while(...) { }) */
  current = *position;
  node = read_statement_while(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try do-while statement (do { } while(...);) */
  current = *position;
  node = read_statement_do_while(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try for statement (for(init; cond; incr) { }) */
  current = *position;
  node = read_statement_for(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try foreach statement (foreach(name : iter) { }) */
  current = *position;
  node = read_statement_foreach(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try switch statement (switch(value) { case(...) -> { } else -> { } }) */
  current = *position;
  node = read_statement_switch(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try import statement (import ...) */
  current = *position;
  node = read_statement_import(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try return statement (return ...;) */
  current = *position;
  node = read_statement_return(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try break statement (break;) */
  current = *position;
  node = read_statement_break(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try continue statement (continue;) */
  current = *position;
  node = read_statement_continue(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try defer statement (defer expr; / defer { }) */
  current = *position;
  node = read_statement_defer(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try empty statement (;) */
  current = *position;
  node = read_statement_empty(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Expression statement is the fallback — it has no distinguishing prefix,
     any expression can start it, so it must be tried last. */
  current = *position;
  node = read_statement_expression(ctx, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  return NULL;
}
