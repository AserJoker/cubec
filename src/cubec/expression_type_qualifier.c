#include "cubec/expression_type_qualifier.h"
#include "core/token.h"
#include "cubec/ast_factory.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/node_error.h"
#include "cubec/token.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_type_qualifier_init(
    cubec_expression_type_qualifier_t self, allocator_t allocator,
    cubec_expression_type_qualifier_init_t *init) {
  if (!init) return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_type.init(&self->super, allocator, &super_init);
  self->type = init->type;
  self->is_const = init->is_const;
  self->is_volatile = init->is_volatile;
}

static void _cubec_expression_type_qualifier_dispose(
    cubec_expression_type_qualifier_t self, allocator_t allocator) {
  allocator_free(allocator, &self->type);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_type_qualifier_clone(
    cubec_expression_type_qualifier_t self, allocator_t allocator,
    cubec_expression_type_qualifier_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->type = value_clone(allocator, another->type);
  self->is_const = another->is_const;
  self->is_volatile = another->is_volatile;
}

static void _cubec_expression_type_qualifier_move(
    cubec_expression_type_qualifier_t self, allocator_t allocator,
    cubec_expression_type_qualifier_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->type = value_move(allocator, another->type);
  self->is_const = another->is_const;
  self->is_volatile = another->is_volatile;
}

type_t g_cubec_expression_type_qualifier_type = {
    .name = "cubec.cubec.expression_type_qualifier",
    .size = sizeof(struct _cubec_expression_type_qualifier_t),
    .init = (type_init_fn_t)_cubec_expression_type_qualifier_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_type_qualifier_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_type_qualifier_clone,
    .move = (type_move_fn_t)_cubec_expression_type_qualifier_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_type_qualifier
 *  Supports chained qualifiers: const volatile T, volatile const T
 *  Each node represents a single qualifier; chains create nested nodes.
 * -------------------------------------------------------------------------- */

node_t read_expression_type_qualifier(context_t ctx, vec_t tokens,
                                      size_t *position, const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  /* Track qualifier order: first seen → outermost, last seen → innermost. */
  typedef enum { _Q_NONE, _Q_CONST, _Q_VOLATILE } _qualifier_kind_t;
  _qualifier_kind_t first = _Q_NONE, second = _Q_NONE;
  location_t start_loc = {0};

  /* Consume one or more const/volatile keywords */
  while (true) {
    token_t kw = vec_get(tokens, current);
    if (!kw || token_get_kind(kw) != CUBEC_TOKEN_KEYWORD) break;
    if (token_is(kw, CUBEC_TOKEN_KEYWORD, "const")) {
      if (first == _Q_NONE) {
        start_loc = *token_get_location(kw);
        first = _Q_CONST;
      } else if (second == _Q_NONE && first != _Q_CONST) {
        second = _Q_CONST;
      }
      current++;
      skip_whitespace(tokens, &current);
      continue;
    }
    if (token_is(kw, CUBEC_TOKEN_KEYWORD, "volatile")) {
      if (first == _Q_NONE) {
        start_loc = *token_get_location(kw);
        first = _Q_VOLATILE;
      } else if (second == _Q_NONE && first != _Q_VOLATILE) {
        second = _Q_VOLATILE;
      }
      current++;
      skip_whitespace(tokens, &current);
      continue;
    }
    break;
  }

  if (first == _Q_NONE) return NULL;
  start_loc.filename = filename;

  /* Parse the underlying type using read_expression_base (greedy). */
  node_t type = read_expression_base(ctx, tokens, &current, filename);
  if (node_is_error(type)) return type;
  if (!type) {
    goto onerror;
  }

  /* Build nested qualifier nodes: second qualifier (innermost) wraps base
   * type first, first qualifier (outermost) wraps last.
   * e.g. const volatile i32 → const(volatile(i32))
   *      volatile const i32 → volatile(const(i32)) */
  node_t result = type;
  if (second == _Q_CONST) {
    location_t loc = start_loc;
    loc.end = result->location.end;
    loc.filename = filename;
    result = allocator_create(allocator, &g_cubec_expression_type_qualifier_type,
                         &(cubec_expression_type_qualifier_init_t){
                             .location = loc, .type = result,
                             .is_const = true, .is_volatile = false});
  } else if (second == _Q_VOLATILE) {
    location_t loc = start_loc;
    loc.end = result->location.end;
    loc.filename = filename;
    result = allocator_create(allocator, &g_cubec_expression_type_qualifier_type,
                         &(cubec_expression_type_qualifier_init_t){
                             .location = loc, .type = result,
                             .is_const = false, .is_volatile = true});
  }
  if (first == _Q_CONST) {
    location_t loc = start_loc;
    loc.end = result->location.end;
    loc.filename = filename;
    result = allocator_create(allocator, &g_cubec_expression_type_qualifier_type,
                         &(cubec_expression_type_qualifier_init_t){
                             .location = loc, .type = result,
                             .is_const = true, .is_volatile = false});
  } else if (first == _Q_VOLATILE) {
    location_t loc = start_loc;
    loc.end = result->location.end;
    loc.filename = filename;
    result = allocator_create(allocator, &g_cubec_expression_type_qualifier_type,
                         &(cubec_expression_type_qualifier_init_t){
                             .location = loc, .type = result,
                             .is_const = false, .is_volatile = true});
  }

  *position = current;
  return result;

onerror:
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                       start_loc, "invalid type qualifier expression");
  ctx->error_count++;
  return cubec_ast_create_error(ctx, start_loc);
}

/* --------------------------------------------------------------------------
 *  Factory: cubec_ast_create_type_qualifier
 * -------------------------------------------------------------------------- */

node_t cubec_ast_create_type_qualifier(context_t ctx, location_t loc,
                                       node_t base, bool is_const,
                                       bool is_volatile) {
  allocator_t alloc = ctx->allocator;
  cubec_expression_type_qualifier_init_t init = {
      .location = loc, .parent = NULL, .type = base,
      .is_const = is_const, .is_volatile = is_volatile};
  return (node_t)allocator_create(
      alloc, &g_cubec_expression_type_qualifier_type, &init);
}
