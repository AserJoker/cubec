#include "cubec/declaration_slice.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "core/token_writer.h"
#include "cubec/expression.h"
#include "cubec/token.h"

static void
_cubec_declaration_slice_init(cubec_declaration_slice_t self,
                              allocator_t allocator,
                              cubec_declaration_slice_init_t *init) {
  if (!init)
    return;
  cubec_declaration_init_t super_init = {
      .kind = CUBEC_NODE_DECLARATION_SLICE,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_cubec_declaration_class.init(&self->super, allocator, &super_init);
  self->type = init->type;
  self->is_const = init->is_const;
  self->is_volatile = init->is_volatile;
}

static void _cubec_declaration_slice_dispose(cubec_declaration_slice_t self,
                                             allocator_t allocator) {
  allocator_free(allocator, &self->type);
  g_cubec_declaration_class.dispose(&self->super, allocator);
}

static void _cubec_declaration_slice_clone(cubec_declaration_slice_t self,
                                           allocator_t allocator,
                                           cubec_declaration_slice_t another) {
  g_cubec_declaration_class.clone(&self->super, allocator, &another->super);
  self->type = alloc_clone(allocator, another->type);
  self->is_const = another->is_const;
  self->is_volatile = another->is_volatile;
}

static void _cubec_declaration_slice_move(cubec_declaration_slice_t self,
                                          allocator_t allocator,
                                          cubec_declaration_slice_t another) {
  g_cubec_declaration_class.move(&self->super, allocator, &another->super);
  self->type = alloc_move(allocator, another->type);
  self->is_const = another->is_const;
  self->is_volatile = another->is_volatile;
}

class_t g_cubec_declaration_slice_class = {
    .name = "cubec.cubec.declaration_slice",
    .size = sizeof(struct _cubec_declaration_slice_t),
    .init = (class_init_fn_t)_cubec_declaration_slice_init,
    .dispose = (class_dispose_fn_t)_cubec_declaration_slice_dispose,
    .clone = (class_clone_fn_t)_cubec_declaration_slice_clone,
    .move = (class_move_fn_t)_cubec_declaration_slice_move,
};

/**
 * Helper function to check if a token is a specific keyword.
 */
static bool _is_keyword(vec_t tokens, size_t position, const char *keyword) {
  token_t token = vec_get(tokens, position);
  if (!token)
    return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD)
    return false;
  return location_is(token_get_location(token), keyword);
}

node_t read_declaration_slice(vm_t vm, vec_t tokens, size_t *position,
                              const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;
  cubec_declaration_slice_t node = NULL;
  node_t type = NULL;
  location_t start_location = {0};
  bool is_const = false;
  bool is_volatile = false;

  /* Expect '[' (slice indicator - must be immediately followed by ']') */
  token_t open_bracket = vec_get(tokens, current);
  if (!token_is(open_bracket, CUBEC_TOKEN_SYMBOL, "[")) {
    return NULL;
  }
  current++;

  /* Expect ']' immediately after '[' - no whitespace, comments, or newlines
   * allowed */
  token_t close_bracket = vec_get(tokens, current);
  if (!token_is(close_bracket, CUBEC_TOKEN_SYMBOL, "]")) {
    return NULL;
  }
  current++;

  start_location = *token_get_location(open_bracket);
  start_location.filename = filename;

  /* Skip whitespace before parsing qualifiers */
  skip_whitespace(tokens, &current);

  /* Check for optional 'const' and 'volatile' qualifiers (any order, may
   * repeat) */
  while (true) {
    if (_is_keyword(tokens, current, "const")) {
      is_const = true;
      current++;
      skip_whitespace(tokens, &current);
      continue;
    }
    if (_is_keyword(tokens, current, "volatile")) {
      is_volatile = true;
      current++;
      skip_whitespace(tokens, &current);
      continue;
    }
    break;
  }

  /* Skip whitespace before parsing the underlying type */
  skip_whitespace(tokens, &current);

  /* Parse the underlying type using read_expression_type (greedy).
   * The slice declaration greedily consumes the full type expression,
   * including ternary: []a ? b : c → slice(ternary(a, b, c)).
   * Use grouping for the alternative: ([] a) ? b : c → ternary(slice(a), b, c).
   * Namespace access binds tighter: []std::vec::Vec → [](std::vec::Vec). */
  type = read_expression_base(vm, tokens, &current, filename);
  if (!type) {
    goto onerror;
  }

  node = allocator_create(allocator, &g_cubec_declaration_slice_class,
                          &(cubec_declaration_slice_init_t){
                              .type = type,
                              .is_const = is_const,
                              .is_volatile = is_volatile,
                          });
  if (!node)
    goto onerror;

  /* Set location from start to end of type */
  node->super.super.super.location = start_location;
  node->super.super.super.location.end = type->location.end;

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &type);
  allocator_free(allocator, &node);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Factory: create_declaration_slice
 * -------------------------------------------------------------------------- */

node_t create_declaration_slice(vm_t vm, location_t loc, node_t base,
                                bool is_const, bool is_volatile) {
  allocator_t alloc = vm_get_allocator(vm);
  cubec_declaration_slice_init_t init = {
      .type = base, .is_const = is_const, .is_volatile = is_volatile};
  return (node_t)allocator_create(alloc, &g_cubec_declaration_slice_class,
                                  &init);
}

void emit_declaration_slice(emit_context_t ctx, node_t node) {
  cubec_declaration_slice_t slice = (cubec_declaration_slice_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_symbol(ctx, "[");
  emit_symbol(ctx, "]");
  if (slice->is_const) {
    emit_keyword(ctx, "const");
    emit_space(ctx);
  }
  if (slice->is_volatile) {
    emit_keyword(ctx, "volatile");
    emit_space(ctx);
  }
  emit_expression(ctx, slice->type);
}