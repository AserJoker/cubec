#include "cubec/statement_enum.h"
#include "core/token.h"
#include "cubec/decorator.h"
#include "cubec/expression_type_enum.h"
#include "cubec/literal_identifier.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_enum_init(cubec_statement_enum_t self,
                                       allocator_t allocator,
                                       cubec_statement_enum_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_ENUM,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->is_export = init->is_export;
  self->name = init->name;
  self->items = init->items;
  self->decorators = init->decorators;
}

static void _cubec_statement_enum_dispose(cubec_statement_enum_t self,
                                          allocator_t allocator) {
  allocator_free(allocator, &self->decorators);
  allocator_free(allocator, &self->items);
  allocator_free(allocator, &self->name);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_enum_clone(cubec_statement_enum_t self,
                                        allocator_t allocator,
                                        cubec_statement_enum_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->is_export = another->is_export;
  self->name = value_clone(allocator, another->name);
  self->items = value_clone(allocator, another->items);
  return;
}

static void _cubec_statement_enum_move(cubec_statement_enum_t self,
                                       allocator_t allocator,
                                       cubec_statement_enum_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->is_export = another->is_export;
  self->name = value_move(allocator, another->name);
  self->items = value_move(allocator, another->items);
  return;
}

type_t g_cubec_statement_enum_type = {
    .name = "cubec.cubec.statement_enum",
    .size = sizeof(struct _cubec_statement_enum_t),
    .init = (type_init_fn_t)_cubec_statement_enum_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_enum_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_enum_clone,
    .move = (type_move_fn_t)_cubec_statement_enum_move,
};

/* --------------------------------------------------------------------------
 *  Helper: check keyword
 * -------------------------------------------------------------------------- */

static bool _is_keyword(vec_t tokens, size_t position, const char *keyword) {
  token_t token = vec_get(tokens, position);
  if (!token)
    return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD)
    return false;
  return location_is(token_get_location(token), keyword);
}

/* --------------------------------------------------------------------------
 *  Parser: read_statement_enum — delegates to read_expression_type_enum
 * -------------------------------------------------------------------------- */

node_t read_statement_enum(context_t ctx, vec_t tokens, size_t *position,
                           const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  bool is_export = false;
  node_t name = NULL;
  node_t expr_node = NULL;
  cubec_statement_enum_t node = NULL;
  location_t start_location = {0};
  vec_t decorators = NULL;

  /* Collect decorators [[...]] */
  {
    while (true) {
      skip_whitespace(tokens, &current);
      node_t dec = read_decorator(ctx, tokens, &current, filename);
      if (node_is_error(dec))
        return dec;
      if (!dec)
        break;
      if (!decorators) {
        decorators =
            allocator_create(allocator, &g_vec_type, &(vec_init_t){true});
      }
      vec_push(decorators, dec);
    }
  }

  /* 1. Parse optional 'export' modifier */
  if (_is_keyword(tokens, current, "export")) {
    is_export = true;
    token_t tok = vec_get(tokens, current);
    start_location = *token_get_location(tok);
    start_location.filename = filename;
    current++;
    skip_whitespace(tokens, &current);
  }

  /* 2. Expect 'enum' keyword */
  if (!_is_keyword(tokens, current, "enum")) {
    goto onerror;
  }
  if (start_location.begin.offset == 0) {
    token_t tok = vec_get(tokens, current);
    start_location = *token_get_location(tok);
    start_location.filename = filename;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse enum name (required for statement form) */
  name = read_literal_identifier(ctx, tokens, &current, filename);
  if (node_is_error(name)) {
    allocator_free(allocator, &decorators);
    return name;
  }
  if (!name)
    goto onerror;

  skip_whitespace(tokens, &current);

  /* 4. Delegate to read_expression_type_enum_body for { items }
   *    (enum keyword already consumed, pass start_location for span) */
  expr_node = read_expression_type_enum_body(ctx, tokens, &current, filename,
                                             start_location);
  if (node_is_error(expr_node)) {
    allocator_free(allocator, &decorators);
    allocator_free(allocator, &name);
    return expr_node;
  }
  if (!expr_node)
    goto onerror;
  cubec_expression_type_enum_t expr_enum =
      (cubec_expression_type_enum_t)expr_node;

  /* 5. Build location (use modifier start or enum keyword location) */
  location_t loc = expr_node->location;
  if (start_location.begin.offset != 0) {
    loc.begin = start_location.begin;
  }

  /* 6. Create statement_enum node, transferring ownership from
   * expression_type_enum */
  cubec_statement_enum_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_export = is_export,
      .name = name,
      .items = expr_enum->items,
      .decorators = decorators,
  };

  /* Nullify fields in expression node to prevent double-free during dispose */
  expr_enum->items = NULL;

  allocator_free(allocator, &expr_node);

  node = allocator_create(allocator, &g_cubec_statement_enum_type, &init);
  *position = current;
  return &node->super;

onerror:
  allocator_free(allocator, &decorators);
  allocator_free(allocator, &name);
  allocator_free(allocator, &expr_node);
  allocator_free(allocator, &node);
  return cubec_ast_create_error(ctx, start_location);
}

node_t cubec_ast_create_enum_stmt(context_t ctx, location_t loc,
                                  const char *name, vec_t items,
                                  bool is_export) {
  allocator_t alloc = ctx->allocator;
  node_t name_node = cubec_ast_create_identifier(ctx, loc, name);
  cubec_statement_enum_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_export = is_export,
      .name = name_node,
      .items = items,
  };
  return (node_t)allocator_create(alloc, &g_cubec_statement_enum_type, &init);
}
