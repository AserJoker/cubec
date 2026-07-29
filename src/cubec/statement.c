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
#include "cubec/statement_test.h"
#include "cubec/statement_export_from.h"
#include "cubec/node_error.h"
#include "cubec/ast_factory.h"
#include "cubec/token.h"

/* ===== Error recovery: sync to next statement boundary ===== */

static bool _is_statement_keyword(token_t tok) {
  if (token_get_kind(tok) != CUBEC_TOKEN_KEYWORD) return false;
  location_t *loc = token_get_location(tok);
  if (!loc) return false;
  /* Check against statement-starting keywords */
  return location_is(loc, "func") || location_is(loc, "if") ||
         location_is(loc, "for") || location_is(loc, "foreach") ||
         location_is(loc, "while") || location_is(loc, "do") ||
         location_is(loc, "var") || location_is(loc, "return") ||
         location_is(loc, "break") || location_is(loc, "continue") ||
         location_is(loc, "struct") || location_is(loc, "enum") ||
         location_is(loc, "union") || location_is(loc, "cunion") ||
         location_is(loc, "interface") || location_is(loc, "import") ||
         location_is(loc, "type") || location_is(loc, "defer") ||
         location_is(loc, "switch") || location_is(loc, "comptime") ||
         location_is(loc, "test") || location_is(loc, "export") ||
         location_is(loc, "extern") || location_is(loc, "inline");
}

static void _sync_to_recovery_point(vec_t tokens, size_t *position) {
  size_t current = *position;
  size_t size = vec_get_size(tokens);
  while (current < size) {
    token_t tok = vec_get(tokens, current);
    if (!tok) { current++; continue; }
    /* Always sync at ';' — consume it and stop */
    if (token_is(tok, CUBEC_TOKEN_SYMBOL, ";")) {
      current++;
      break;
    }
    /* Sync at '}' — do NOT consume (it closes the enclosing block) */
    if (token_is(tok, CUBEC_TOKEN_SYMBOL, "}")) {
      break;
    }
    /* Sync at statement-starting keywords — do NOT consume */
    if (_is_statement_keyword(tok)) {
      break;
    }
    /* Sync at EOF */
    if (token_get_kind(tok) == CUBEC_TOKEN_EOF) {
      break;
    }
    current++;
  }
  *position = current;
}

/* ===== Statement dispatcher ===== */

node_t read_statement(context_t ctx, vec_t tokens, size_t *position,
                      const char *filename) {
  size_t start = *position;
  skip_whitespace(tokens, &start);
  token_t start_token = vec_get(tokens, start);
  location_t start_location = start_token ? *token_get_location(start_token) : (location_t){0};
  start_location.filename = filename;

  size_t current;
  node_t node;
  bool had_error = false; /* track if any sub-parser returned an Error */

  /* Try specific statement types first (they have distinguishing prefixes).
     When a sub-parser returns an Error node, free it, reset position, and
     continue trying — another sub-parser may succeed from the same position. */

  /* Try block statement ({...}) */
  current = *position;
  node = read_statement_block(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try comptime statement (comptime { } / comptime if / comptime for) —
     must be before declaration/function since 'comptime' is also a modifier */
  current = *position;
  node = read_statement_comptime(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try declaration statement (var ...) */
  current = *position;
  node = read_statement_declaration(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try type declaration statement (type ...) */
  current = *position;
  node = read_statement_declaration_type(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try function statement (func ... / export func ... / inline func ... / extern func ...) */
  current = *position;
  node = read_statement_function(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try interface statement (interface ... / export interface ...) */
  current = *position;
  node = read_statement_interface(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try struct statement (struct ... / export struct ...) */
  current = *position;
  node = read_statement_struct(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try enum statement (enum ... / export enum ...) */
  current = *position;
  node = read_statement_enum(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try cunion statement (cunion ...) */
  current = *position;
  node = read_statement_cunion(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try union statement (union ... / export union ...) */
  current = *position;
  node = read_statement_union(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try if statement (if(...) { } else ...) */
  current = *position;
  node = read_statement_if(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try while statement (while(...) { }) */
  current = *position;
  node = read_statement_while(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try do-while statement (do { } while(...);) */
  current = *position;
  node = read_statement_do_while(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try for statement (for(init; cond; incr) { }) */
  current = *position;
  node = read_statement_for(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try foreach statement (foreach(name : iter) { }) */
  current = *position;
  node = read_statement_foreach(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try switch statement (switch(value) { case(...) -> { } else -> { } }) */
  current = *position;
  node = read_statement_switch(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try import statement (import ...) */
  current = *position;
  node = read_statement_import(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try export-from statement (export * from "..."; / export { ... } from "...";) */
  current = *position;
  node = read_statement_export_from(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try return statement (return ...;) */
  current = *position;
  node = read_statement_return(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try break statement (break;) */
  current = *position;
  node = read_statement_break(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try continue statement (continue;) */
  current = *position;
  node = read_statement_continue(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try defer statement (defer expr; / defer { }) */
  current = *position;
  node = read_statement_defer(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try test statement (test "name" { }) */
  current = *position;
  node = read_statement_test(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Try empty statement (;) */
  current = *position;
  node = read_statement_empty(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* Expression statement is the fallback — it has no distinguishing prefix,
     any expression can start it, so it must be tried last. */
  current = *position;
  node = read_statement_expression(ctx, tokens, &current, filename);
  if (node_is_error(node)) { allocator_free(ctx->allocator, &node); had_error = true; }
  else if (node) { *position = current; return node; }

  /* All sub-parsers returned NULL — check if any had returned Error */
  if (had_error) {
    /* At least one sub-parser recognized a prefix but failed to complete.
       Sync past the bad tokens and return a StatementError placeholder. */
    size_t sync_pos = start;
    _sync_to_recovery_point(tokens, &sync_pos);
    if (sync_pos <= start) sync_pos = start + 1; /* ensure progress */
    *position = sync_pos;
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, start_location,
                         "invalid statement");
    ctx->error_count++;
    return cubec_ast_create_error_stmt(ctx, start_location);
  }

  return NULL;
}
