#include "cubec/statement_break.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_break_init(cubec_statement_break_t self,
                                        allocator_t allocator,
                                        cubec_statement_break_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_BREAK,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_class.init(&self->super, allocator, &super_init);
}

static void _cubec_statement_break_dispose(cubec_statement_break_t self,
                                           allocator_t allocator) {
  g_node_class.dispose(&self->super, allocator);
}

static void _cubec_statement_break_clone(cubec_statement_break_t self,
                                         allocator_t allocator,
                                         cubec_statement_break_t another) {
  g_node_class.clone(&self->super, allocator, &another->super);
  return;
}

static void _cubec_statement_break_move(cubec_statement_break_t self,
                                        allocator_t allocator,
                                        cubec_statement_break_t another) {
  g_node_class.move(&self->super, allocator, &another->super);
  return;
}

class_t g_cubec_statement_break_class = {
    .name = "cubec.cubec.statement_break",
    .size = sizeof(struct _cubec_statement_break_t),
    .init = (class_init_fn_t)_cubec_statement_break_init,
    .dispose = (class_dispose_fn_t)_cubec_statement_break_dispose,
    .clone = (class_clone_fn_t)_cubec_statement_break_clone,
    .move = (class_move_fn_t)_cubec_statement_break_move,
};

/* --------------------------------------------------------------------------
 *  Helper: check keyword / symbol
 * -------------------------------------------------------------------------- */

static bool _is_keyword(vec_t tokens, size_t position, const char *keyword) {
  token_t token = vec_get(tokens, position);
  if (!token)
    return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD)
    return false;
  return location_is(token_get_location(token), keyword);
}

static bool _is_symbol(vec_t tokens, size_t position, const char *symbol) {
  token_t token = vec_get(tokens, position);
  if (!token)
    return false;
  return token_is(token, CUBEC_TOKEN_SYMBOL, symbol);
}

/* --------------------------------------------------------------------------
 *  Parser: read_statement_break — break;
 * -------------------------------------------------------------------------- */

node_t read_statement_break(vm_t vm, vec_t tokens, size_t *position,
                            const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;

  /* 1. Expect 'break' keyword */
  if (!_is_keyword(tokens, current, "break")) {
    return NULL;
  }
  token_t break_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(break_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Expect ';' */
  if (!_is_symbol(tokens, current, ";")) {
    goto onerror;
  }
  token_t semi = vec_get(tokens, current);
  current++;

  /* 3. Build location */
  location_t loc = start_location;
  loc.end = token_get_location(semi)->end;

  cubec_statement_break_init_t init = {
      .location = loc,
      .parent = NULL,
  };
  cubec_statement_break_t node =
      allocator_create(allocator, &g_cubec_statement_break_class, &init);
  *position = current;
  return &node->super;

onerror:
  diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR, start_location,
                       "expected ';' after 'break'");
  return create_error(vm, start_location);
}

node_t create_statement_break(vm_t vm, location_t loc) {
  allocator_t alloc = vm_get_allocator(vm);
  cubec_statement_break_init_t init = {.location = loc, .parent = NULL};
  return (node_t)allocator_create(alloc, &g_cubec_statement_break_class, &init);
}

void emit_statement_break(emit_context_t ctx, node_t node) {
  recover_comments_to(ctx, node->location.begin.offset);
  emit_keyword(ctx, "break");
  emit_symbol(ctx, ";");
}