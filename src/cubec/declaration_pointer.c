#include "cubec/declaration_pointer.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/token.h"

static void
_cubec_declaration_pointer_init(cubec_declaration_pointer_t self,
                                allocator_t allocator,
                                cubec_declaration_pointer_init_t *init) {
  if (!init)
    return;
  cubec_declaration_init_t super_init = {
      .kind = CUBEC_NODE_DECLARATION_POINTER,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_cubec_declaration_type.init(&self->super, allocator, &super_init);
  self->type = init->type;
  self->is_const = init->is_const;
  self->is_volatile = init->is_volatile;
}

static void _cubec_declaration_pointer_dispose(cubec_declaration_pointer_t self,
                                               allocator_t allocator) {
  allocator_free(allocator, &self->type);
  g_cubec_declaration_type.dispose(&self->super, allocator);
}

static void
_cubec_declaration_pointer_clone(cubec_declaration_pointer_t self,
                                 allocator_t allocator,
                                 cubec_declaration_pointer_t another) {
  g_cubec_declaration_type.clone(&self->super, allocator, &another->super);
  self->type = value_clone(allocator, another->type);
  self->is_const = another->is_const;
  self->is_volatile = another->is_volatile;
}

static void
_cubec_declaration_pointer_move(cubec_declaration_pointer_t self,
                                allocator_t allocator,
                                cubec_declaration_pointer_t another) {
  g_cubec_declaration_type.move(&self->super, allocator, &another->super);
  self->type = value_move(allocator, another->type);
  self->is_const = another->is_const;
  self->is_volatile = another->is_volatile;
}

type_t g_cubec_declaration_pointer_type = {
    .name = "cubec.cubec.declaration_pointer",
    .size = sizeof(struct _cubec_declaration_pointer_t),
    .init = (type_init_fn_t)_cubec_declaration_pointer_init,
    .dispose = (type_dispose_fn_t)_cubec_declaration_pointer_dispose,
    .clone = (type_clone_fn_t)_cubec_declaration_pointer_clone,
    .move = (type_move_fn_t)_cubec_declaration_pointer_move,
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

node_t read_declaration_pointer(context_t ctx, vec_t tokens, size_t *position,
                                const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  cubec_declaration_pointer_t node = NULL;
  node_t type = NULL;
  location_t start_location = {0};
  bool is_const = false;
  bool is_volatile = false;

  /* Expect '*' (pointer indicator) */
  token_t star_token = vec_get(tokens, current);
  if (!token_is(star_token, CUBEC_TOKEN_SYMBOL, "*")) {
    return NULL;
  }
  current++;
  start_location = *token_get_location(star_token);
  start_location.filename = filename;

  /* Skip whitespace after '*' */
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

  /* Parse the underlying type using read_expression_base (greedy for ternary,
   * but not comma/assignment — prevents consuming commas in comma-separated
   * contexts like function parameter lists).
   * The pointer declaration greedily consumes the type expression including
   * ternary: *a ? b : c → pointer(ternary(a, b, c)).
   * Use grouping for the alternative: (* a) ? b : c → ternary(pointer(a), b,
   * c). Namespace access binds tighter: *std::vec::Vec → *(std::vec::Vec). */
  type = read_expression_base(ctx, tokens, &current, filename);
  if (!type) {
    goto onerror;
  }

  node = allocator_create(allocator, &g_cubec_declaration_pointer_type,
                          &(cubec_declaration_pointer_init_t){
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
 *  Factory: create_declaration_pointer
 * -------------------------------------------------------------------------- */

node_t create_declaration_pointer(context_t ctx, location_t loc, node_t base,
                                  bool is_const, bool is_volatile) {
  allocator_t alloc = ctx->allocator;
  cubec_declaration_pointer_init_t init = {
      .type = base, .is_const = is_const, .is_volatile = is_volatile};
  return (node_t)allocator_create(alloc, &g_cubec_declaration_pointer_type,
                                  &init);
}

void emit_declaration_pointer(emit_context_t ctx, node_t node) {
  cubec_declaration_pointer_t pointer = (cubec_declaration_pointer_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_symbol(ctx, "*");
  if (pointer->is_const) {
    emit_keyword(ctx, "const");
    emit_space(ctx);
  }
  if (pointer->is_volatile) {
    emit_keyword(ctx, "volatile");
    emit_space(ctx);
  }
  emit_expression(ctx, pointer->type);
}