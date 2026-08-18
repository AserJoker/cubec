#include "cubec/declaration_callable.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "core/token_writer.h"
#include "core/vec.h"
#include "cubec/expression.h"
#include "cubec/expression_spread.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void
_cubec_declaration_callable_init(cubec_declaration_callable_t self,
                                allocator_t allocator,
                                cubec_declaration_callable_init_t *init) {
  if (!init)
    return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_DECLARATION_CALLABLE,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_class.init(&self->super, allocator, &super_init);
  self->parameters = init->parameters;
  self->return_type = init->return_type;
  self->is_c_variadic = init->is_c_variadic;
}

static void _cubec_declaration_callable_dispose(cubec_declaration_callable_t self,
                                               allocator_t allocator) {
  allocator_free(allocator, &self->return_type);
  allocator_free(allocator, &self->parameters);
  g_cubec_expression_class.dispose(&self->super, allocator);
}

static void
_cubec_declaration_callable_clone(cubec_declaration_callable_t self,
                                 allocator_t allocator,
                                 cubec_declaration_callable_t another) {
  g_cubec_expression_class.clone(&self->super, allocator, &another->super);
  self->parameters = alloc_clone(allocator, another->parameters);
  self->return_type = another->return_type
                          ? alloc_clone(allocator, another->return_type)
                          : NULL;
  self->is_c_variadic = another->is_c_variadic;
  return;

cleanup:
  allocator_free(allocator, &self->parameters);
  allocator_free(allocator, &self->return_type);
}

static void
_cubec_declaration_callable_move(cubec_declaration_callable_t self,
                                allocator_t allocator,
                                cubec_declaration_callable_t another) {
  g_cubec_expression_class.move(&self->super, allocator, &another->super);
  self->parameters = alloc_move(allocator, another->parameters);
  self->return_type =
      another->return_type ? alloc_move(allocator, another->return_type) : NULL;
  self->is_c_variadic = another->is_c_variadic;
  return;

cleanup:
  allocator_free(allocator, &self->parameters);
  allocator_free(allocator, &self->return_type);
}

class_t g_cubec_declaration_callable_class = {
    .name = "cubec.cubec.declaration_callable",
    .size = sizeof(struct _cubec_declaration_callable_t),
    .init = (class_init_fn_t)_cubec_declaration_callable_init,
    .dispose = (class_dispose_fn_t)_cubec_declaration_callable_dispose,
    .clone = (class_clone_fn_t)_cubec_declaration_callable_clone,
    .move = (class_move_fn_t)_cubec_declaration_callable_move,
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
 *  Parser: read_declaration_callable
 * -------------------------------------------------------------------------- */

node_t read_declaration_callable(vm_t vm, vec_t tokens, size_t *position,
                                const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;
  vec_t parameters = NULL;
  node_t return_type = NULL;
  bool is_c_variadic = false;
  cubec_declaration_callable_t node = NULL;

  /* 1. Expect 'func' keyword */
  if (!_is_keyword(tokens, current, "func")) {
    return NULL;
  }
  token_t func_token = vec_get(tokens, current);
  if (!func_token)
    return NULL;
  location_t start_location = *token_get_location(func_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Expect '(' */
  token_t open_paren = vec_get(tokens, current);
  if (!open_paren || !token_is(open_paren, CUBEC_TOKEN_SYMBOL, "(")) {
    /* Not a function type — could be a function expression in value context.
     * Return NULL to let other parsers handle it. */
    return NULL;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse parameter list (type-only, no names).
   * Function type parameters are just types: func(i32, f64) -> bool.
   * Function expression parameters have names: func(x: i32, y: f64): bool { ...
   * }. If we encounter a named parameter pattern (identifier followed by ':'),
   * this is a function expression, not a function type — return NULL. */
  parameters = allocator_create(allocator, &g_vec_class, &(vec_init_t){true});

  if (_is_symbol(tokens, current, ")")) {
    /* no parameters */
  } else {
    /* Parse type parameters.
     * Check for named parameter pattern: if the first token is an identifier
     * followed by ':', this is a function expression, not a function type. */
    {
      size_t lookahead = current;
      /* Skip past '...' if present for lookahead */
      bool has_prefix_dots = _is_symbol(tokens, lookahead, "...");
      if (has_prefix_dots) {
        lookahead++;
        skip_whitespace(tokens, &lookahead);
      }
      token_t first = vec_get(tokens, lookahead);
      if (first && token_get_kind(first) == CUBEC_TOKEN_IDENTIFIER) {
        lookahead++;
        skip_whitespace(tokens, &lookahead);
        token_t colon = vec_get(tokens, lookahead);
        if (colon && token_is(colon, CUBEC_TOKEN_SYMBOL, ":")) {
          /* Named parameter pattern — this is a function expression */
          allocator_free(allocator, &parameters);
          return NULL;
        }
      }
    }

    while (true) {
      /* Check for pack spread (...) or C-style variadic */
      if (_is_symbol(tokens, current, "...")) {
        size_t save_pos = current;
        current++; /* skip '...' */
        skip_whitespace(tokens, &current);
        /* If next token can be a type expression, it's a pack spread */
        node_t inner = read_expression_base(vm, tokens, &current, filename);
        if (inner) {
          /* Pack spread: ...Args — wrap in expression_spread node */
          cubec_expression_spread_init_t spread_init = {
              .location = inner->location,
              .parent = NULL,
              .value = inner,
          };
          /* Update location to include the '...' */
          token_t first_dot = vec_get(tokens, save_pos);
          if (first_dot) {
            location_t *dot_loc = token_get_location(first_dot);
            spread_init.location.begin = dot_loc->begin;
            spread_init.location.filename = filename;
          }
          node_t spread = allocator_create(
              allocator, &g_cubec_expression_spread_class, &spread_init);
          vec_push(parameters, spread);
          skip_whitespace(tokens, &current);

          token_t comma_or_close = vec_get(tokens, current);
          if (!comma_or_close) {
            goto onerror;
          }
          if (token_is(comma_or_close, CUBEC_TOKEN_SYMBOL, ",")) {
            current++;
            skip_whitespace(tokens, &current);
            continue;
          } else if (token_is(comma_or_close, CUBEC_TOKEN_SYMBOL, ")")) {
            break;
          } else {
            goto onerror;
          }
        } else {
          /* C-style variadic with no named params */
          is_c_variadic = true;
          current = save_pos + 1; /* past the '...' */
          skip_whitespace(tokens, &current);
          break;
        }
      }

      node_t param = read_expression_base(vm, tokens, &current, filename);
      if (!param) {
        goto onerror;
      }
      vec_push(parameters, param);
      skip_whitespace(tokens, &current);

      token_t comma_or_close = vec_get(tokens, current);
      if (!comma_or_close) {
        goto onerror;
      }

      if (token_is(comma_or_close, CUBEC_TOKEN_SYMBOL, ",")) {
        current++;
        skip_whitespace(tokens, &current);
        /* Check for '...' after comma — could be pack spread or C variadic */
        if (_is_symbol(tokens, current, "...")) {
          /* Will be handled in next iteration */
          continue;
        }
      } else if (token_is(comma_or_close, CUBEC_TOKEN_SYMBOL, ")")) {
        break;
      } else {
        goto onerror;
      }
    }
  }

  /* 4. Expect ')' */
  token_t close_paren = vec_get(tokens, current);
  if (!close_paren || !token_is(close_paren, CUBEC_TOKEN_SYMBOL, ")")) {
    location_t *loc = close_paren ? token_get_location(close_paren) : NULL;
    if (loc) {
      goto onerror;
    } else {
      goto onerror;
    }
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 5. Expect '->' (function type uses -> for return type).
   * If ':' follows instead, this is a function expression — return NULL. */
  if (_is_symbol(tokens, current, ":")) {
    /* ':' indicates function expression (func(params): type { body }) */
    allocator_free(allocator, &parameters);
    return NULL;
  }
  if (!_is_symbol(tokens, current, "->")) {
    location_t *loc = NULL;
    token_t tok = vec_get(tokens, current);
    if (tok)
      loc = token_get_location(tok);
    if (loc) {
      goto onerror;
    } else {
      goto onerror;
    }
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 6. Parse return type (greedy — consumes ternary/constraint, but not comma).
   * func(i32) -> A ? B : C → func(i32) -> ternary(A, B, C).
   * Use grouping for the alternative: (func(i32) -> A) ? B : C. */
  return_type = read_expression_base(vm, tokens, &current, filename);
  if (node_is_error(return_type))
    goto onerror;
  if (!return_type) {
    goto onerror;
  }

  /* 7. Build location */
  location_t loc = {
      .begin = start_location.begin,
      .end = return_type->location.end,
      .filename = filename,
  };

  /* 8. Create node */
  node = allocator_create(allocator, &g_cubec_declaration_callable_class,
                          &(cubec_declaration_callable_init_t){
                              .location = loc,
                              .parameters = parameters,
                              .return_type = return_type,
                              .is_c_variadic = is_c_variadic,
                          });

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &return_type);
  allocator_free(allocator, &parameters);
  allocator_free(allocator, &node);
  return create_error(vm, start_location);
}

/* --------------------------------------------------------------------------
 *  Factory: create_declaration_callable
 * -------------------------------------------------------------------------- */

node_t create_declaration_callable(vm_t vm, location_t loc,
                                  vec_t parameters, node_t return_type,
                                  bool is_c_variadic) {
  allocator_t alloc = vm_get_allocator(vm);
  cubec_declaration_callable_init_t init = {
      .location = loc,
      .parent = NULL,
      .parameters = parameters,
      .return_type = return_type,
      .is_c_variadic = is_c_variadic,
  };
  return (node_t)allocator_create(alloc, &g_cubec_declaration_callable_class,
                                  &init);
}

void emit_declaration_callable(emit_context_t ctx, node_t node) {
  cubec_declaration_callable_t expr = (cubec_declaration_callable_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_keyword(ctx, "func");
  emit_symbol(ctx, "(");
  for (size_t idx = 0; idx < vec_get_size(expr->parameters); idx++) {
    if (idx != 0) {
      emit_symbol(ctx, ",");
      emit_space(ctx);
    }
    emit_expression(ctx, vec_get(expr->parameters, idx));
  }
  if (expr->is_c_variadic) {
    emit_symbol(ctx, ",");
    emit_space(ctx);
    emit_symbol(ctx, "...");
  }
  emit_symbol(ctx, ")");
  emit_space(ctx);
  emit_symbol(ctx, "->");
  emit_space(ctx);
  emit_expression(ctx, expr->return_type);
}