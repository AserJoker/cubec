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
  g_cubec_declaration_class.init(&self->super, allocator, &super_init);
  self->type = init->type;
  self->is_volatile = init->is_volatile;
}

static void _cubec_declaration_pointer_dispose(cubec_declaration_pointer_t self,
                                               allocator_t allocator) {
  allocator_free(allocator, &self->type);
  g_cubec_declaration_class.dispose(&self->super, allocator);
}

static void
_cubec_declaration_pointer_clone(cubec_declaration_pointer_t self,
                                 allocator_t allocator,
                                 cubec_declaration_pointer_t another) {
  g_cubec_declaration_class.clone(&self->super, allocator, &another->super);
  self->type = alloc_clone(allocator, another->type);
  self->is_volatile = another->is_volatile;
}

static void
_cubec_declaration_pointer_move(cubec_declaration_pointer_t self,
                                allocator_t allocator,
                                cubec_declaration_pointer_t another) {
  g_cubec_declaration_class.move(&self->super, allocator, &another->super);
  self->type = alloc_move(allocator, another->type);
  self->is_volatile = another->is_volatile;
}

class_t g_cubec_declaration_pointer_class = {
    .name = "cubec.cubec.declaration_pointer",
    .size = sizeof(struct _cubec_declaration_pointer_t),
    .init = (class_init_fn_t)_cubec_declaration_pointer_init,
    .dispose = (class_dispose_fn_t)_cubec_declaration_pointer_dispose,
    .clone = (class_clone_fn_t)_cubec_declaration_pointer_clone,
    .move = (class_move_fn_t)_cubec_declaration_pointer_move,
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

node_t read_declaration_pointer(vm_t vm, vec_t tokens, size_t *position,
                                const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;
  cubec_declaration_pointer_t node = NULL;
  node_t type = NULL;
  location_t start_location = {0};
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

  /* Check for optional 'volatile' qualifier after *.
   * Note: 'const' after * is NOT a pointer qualifier — it belongs to the
   * base type. Use `const * T` (qualifier wrapping pointer) for a const
   * pointer. */
  if (_is_keyword(tokens, current, "volatile")) {
    is_volatile = true;
    current++;
    skip_whitespace(tokens, &current);
  }

  /* Parse the underlying type using read_expression_base (greedy for ternary,
   * but not comma/assignment — prevents consuming commas in comma-separated
   * contexts like function parameter lists).
   * The pointer declaration greedily consumes the type expression including
   * ternary: *a ? b : c → pointer(ternary(a, b, c)).
   * Use grouping for the alternative: (* a) ? b : c → ternary(pointer(a), b,
   * c). Namespace access binds tighter: *std::vec::Vec → *(std::vec::Vec). */
  type = read_expression_base(vm, tokens, &current, filename);
  if (!type) {
    goto onerror;
  }

  node = allocator_create(allocator, &g_cubec_declaration_pointer_class,
                          &(cubec_declaration_pointer_init_t){
                              .type = type,
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

node_t create_declaration_pointer(vm_t vm, location_t loc, node_t base,
                                  bool is_volatile) {
  allocator_t alloc = vm_get_allocator(vm);
  cubec_declaration_pointer_init_t init = {
      .type = base, .is_volatile = is_volatile};
  return (node_t)allocator_create(alloc, &g_cubec_declaration_pointer_class,
                                  &init);
}

void emit_declaration_pointer(emit_context_t ctx, node_t node) {
  cubec_declaration_pointer_t pointer = (cubec_declaration_pointer_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_symbol(ctx, "*");
  if (pointer->is_volatile) {
    emit_space(ctx);
    emit_keyword(ctx, "volatile");
  }
  emit_space(ctx);
  emit_expression(ctx, pointer->type);
}