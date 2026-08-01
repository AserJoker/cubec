#include "cubec/expression_initialize_list.h"
#include "core/token.h"
#include "cubec/expression_initialize_field.h"
#include "cubec/expression_spread.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_initialize_list_init(
    cubec_expression_initialize_list_t self, allocator_t allocator,
    cubec_expression_initialize_list_init_t *init) {
  if (!init) return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_INITIALIZE_LIST,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_type.init(&self->super, allocator, &super_init);

  self->type = init->type;
  self->is_field = init->is_field;
  if (init->items) {
    self->items = init->items;
  } else {
    self->items = allocator_create(allocator, &g_vec_type, &(vec_init_t){true});
  }

}

static void _cubec_expression_initialize_list_dispose(
    cubec_expression_initialize_list_t self, allocator_t allocator) {
  allocator_free(allocator, &self->type);
  allocator_free(allocator, &self->items);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_initialize_list_clone(
    cubec_expression_initialize_list_t self, allocator_t allocator,
    cubec_expression_initialize_list_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->type = value_clone(allocator, another->type);
  self->items = value_clone(allocator, another->items);
  self->is_field = another->is_field;
  return;

cleanup:
  allocator_free(allocator, &self->items);
  allocator_free(allocator, &self->type);
}

static void _cubec_expression_initialize_list_move(
    cubec_expression_initialize_list_t self, allocator_t allocator,
    cubec_expression_initialize_list_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->type = value_move(allocator, another->type);
  self->is_field = another->is_field;

  allocator_free(allocator, &self->items);
  self->items = another->items;
  another->items = allocator_create(allocator, &g_vec_type, &(vec_init_t){true});
  return;

cleanup:
  allocator_free(allocator, &self->items);
  allocator_free(allocator, &self->type);
}

type_t g_cubec_expression_initialize_list_type = {
    .name = "cubec.cubec.expression_initialize_list",
    .size = sizeof(struct _cubec_expression_initialize_list_t),
    .init = (type_init_fn_t)_cubec_expression_initialize_list_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_initialize_list_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_initialize_list_clone,
    .move = (type_move_fn_t)_cubec_expression_initialize_list_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_initialize_list
 * -------------------------------------------------------------------------- */

/**
 * Try to parse an initialize list expression:
 *   .<type>{<items>}   — typed, e.g. .Vec{1, 2, 3}
 *   .{<items>}         — anonymous, e.g. .{.x=1, .y=2}
 *
 * Items are comma-separated and must be homogeneous:
 *   - All initialize_field: .name = value, .name2 = value2
 *   - All positional expressions: expr1, expr2
 * Mixing field and positional items is an error.
 */
node_t read_expression_initialize_list(context_t ctx, vec_t tokens,
                                       size_t *position,
                                       const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  cubec_expression_initialize_list_t node = NULL;
  node_t type = NULL;
  vec_t items = NULL;
  token_t dot_token = NULL;
  location_t dot_location = {0};

  /* Expect '.' */
  dot_token = vec_get(tokens, current);
  if (!token_is(dot_token, CUBEC_TOKEN_SYMBOL, ".")) {
    return NULL;
  }
  dot_location = *token_get_location(dot_token);
  current++;

  /* Look ahead: identifier + '{' → typed, '{' → anonymous, else → not ours */
  skip_whitespace(tokens, &current);
  token_t next = vec_get(tokens, current);

  if (token_is(next, CUBEC_TOKEN_SYMBOL, "{")) {
    /* Anonymous: .{ ... } — type stays NULL */
    current++;
  } else {
    /* Possible typed: .<type>{ ... } — type is a type expression */
    type = read_expression_type(ctx, tokens, &current, filename);
    if (!type) {
      return NULL;
    }

    skip_whitespace(tokens, &current);
    token_t brace = vec_get(tokens, current);
    if (!token_is(brace, CUBEC_TOKEN_SYMBOL, "{")) {
      /* .<type> but no '{' — not an initialize_list, backtrack */
      allocator_free(allocator, &type);
      return NULL;
    }
    current++; /* consumed '{' */
  }

  /* We have consumed up to and including '{'. Now parse items. */
  items = allocator_create(allocator, &g_vec_type, &(vec_init_t){true});

  bool mode_determined = false;
  bool is_field_mode = false;

  while (true) {
    skip_whitespace(tokens, &current);
    token_t tok = vec_get(tokens, current);

    /* Check for closing '}' */
    if (token_is(tok, CUBEC_TOKEN_SYMBOL, "}")) {
      current++;
      break;
    }

    /* Parse one item */
    if (!mode_determined) {
      /* Try initialize_field first (only if starts with '.' + identifier + '=') */
      size_t field_pos = current;
      node_t field_item =
          read_expression_initialize_field(ctx, tokens, &field_pos, filename);
      if (field_item) {
        is_field_mode = true;
        mode_determined = true;
        vec_push(items, field_item);
        current = field_pos;
      } else {
        /* Fall back to positional expression: try spread first, then regular */
        is_field_mode = false;
        mode_determined = true;
        node_t expr_item = read_expression_spread(ctx, tokens, &current, filename);
        if (!expr_item) {
          expr_item = read_expression_base(ctx, tokens, &current, filename);
        }
        if (!expr_item) {
          goto onerror;
        }
        vec_push(items, expr_item);
      }
    } else {
      /* Mode already determined */
      if (is_field_mode) {
        node_t field_item =
            read_expression_initialize_field(ctx, tokens, &current, filename);
        if (!field_item) {
          /* In field mode, non-field item is an error (mixed items) */
          goto onerror;
        }
        vec_push(items, field_item);
      } else {
        /* Positional mode: check if next looks like a field (error if so) */
        size_t peek = current;
        token_t peek_dot = vec_get(tokens, peek);
        if (peek_dot && token_is(peek_dot, CUBEC_TOKEN_SYMBOL, ".")) {
          /* Peek further: identifier + '=' → it's a field in positional mode = error */
          size_t field_test = current;
          node_t field_test_item =
              read_expression_initialize_field(ctx, tokens, &field_test, filename);
          if (field_test_item) {
            allocator_free(allocator, &field_test_item);
            goto onerror;
          }
        }
        node_t expr_item = read_expression_spread(ctx, tokens, &current, filename);
        if (!expr_item) {
          expr_item = read_expression_base(ctx, tokens, &current, filename);
        }
        if (!expr_item) {
          goto onerror;
        }
        vec_push(items, expr_item);
      }
    }

    /* After an item, expect ',' or '}' */
    skip_whitespace(tokens, &current);
    tok = vec_get(tokens, current);
    if (token_is(tok, CUBEC_TOKEN_SYMBOL, ",")) {
      current++;
      skip_whitespace(tokens, &current);
      /* Trailing comma: if next is '}', just close the list */
      token_t after_comma = vec_get(tokens, current);
      if (token_is(after_comma, CUBEC_TOKEN_SYMBOL, "}")) {
        current++;
        break;
      }
    } else if (token_is(tok, CUBEC_TOKEN_SYMBOL, "}")) {
      current++;
      break;
    } else {
      goto onerror;
    }
  }

  /* Build node */
  node = allocator_create(allocator, &g_cubec_expression_initialize_list_type,
                                   &(cubec_expression_initialize_list_init_t){
                                       .type = type,
                                       .items = items,
                                       .is_field = is_field_mode,
                                   });
  /* NOTE: items ownership transferred to node via init — do NOT free here */

  /* Location spans from '.' to '}' */
  {
    token_t close_brace = vec_get(tokens, current - 1);
    location_t loc = dot_location;
    loc.end = token_get_location(close_brace)->end;
    node->super.super.location = loc;
    node->super.super.location.filename = filename;
  }

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &items);
  allocator_free(allocator, &type);
  allocator_free(allocator, &node);
  return cubec_ast_create_error(ctx, dot_location);
}

/* --------------------------------------------------------------------------
 *  Factory: cubec_ast_create_initialize_list
 * -------------------------------------------------------------------------- */

node_t cubec_ast_create_initialize_list(context_t ctx, location_t loc,
                                        node_t type, vec_t items,
                                        bool is_field) {
  allocator_t alloc = ctx->allocator;
      cubec_expression_initialize_list_init_t init = {
      .location = loc, .parent = NULL, .type = type, .items = items,
      .is_field = is_field};
  return (node_t)allocator_create(
      alloc, &g_cubec_expression_initialize_list_type, &init);
}
