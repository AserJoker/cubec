#include "cubec/statement_cunion.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "cubec/decorator.h"
#include "cubec/expression.h"
#include "cubec/literal_identifier.h"
#include "cubec/node_error.h"
#include "cubec/struct_field.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_cunion_init(cubec_statement_cunion_t self,
                                         allocator_t allocator,
                                         cubec_statement_cunion_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_CUNION,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->name = init->name;
  self->fields = init->fields;
  self->decorators = init->decorators;
}

static void _cubec_statement_cunion_dispose(cubec_statement_cunion_t self,
                                            allocator_t allocator) {
  allocator_free(allocator, &self->decorators);
  allocator_free(allocator, &self->fields);
  allocator_free(allocator, &self->name);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_cunion_clone(cubec_statement_cunion_t self,
                                          allocator_t allocator,
                                          cubec_statement_cunion_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->name = value_clone(allocator, another->name);
  self->fields = value_clone(allocator, another->fields);
  return;
}

static void _cubec_statement_cunion_move(cubec_statement_cunion_t self,
                                         allocator_t allocator,
                                         cubec_statement_cunion_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->name = value_move(allocator, another->name);
  self->fields = value_move(allocator, another->fields);
  return;
}

type_t g_cubec_statement_cunion_type = {
    .name = "cubec.cubec.statement_cunion",
    .size = sizeof(struct _cubec_statement_cunion_t),
    .init = (type_init_fn_t)_cubec_statement_cunion_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_cunion_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_cunion_clone,
    .move = (type_move_fn_t)_cubec_statement_cunion_move,
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
 *  Parser: read_statement_cunion — cunion <name> { <fields> }
 * -------------------------------------------------------------------------- */

node_t read_statement_cunion(context_t ctx, vec_t tokens, size_t *position,
                             const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  node_t name = NULL;
  vec_t fields = NULL;
  cubec_statement_cunion_t node = NULL;
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

  /* 1. Expect 'cunion' keyword — if not present, return NULL (not our
   * statement) */
  if (!_is_keyword(tokens, current, "cunion")) {
    return NULL;
  }
  token_t cunion_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(cunion_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Parse cunion name (required) */
  name = read_literal_identifier(ctx, tokens, &current, filename);
  if (node_is_error(name)) {
    allocator_free(allocator, &decorators);
    return name;
  }
  if (!name)
    goto onerror;

  skip_whitespace(tokens, &current);

  /* 3. Expect '{' */
  if (!_is_symbol(tokens, current, "{")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 4. Parse fields — semicolon-separated struct_field nodes */
  fields = allocator_create(allocator, &g_vec_type, &(vec_init_t){true});
  while (!_is_symbol(tokens, current, "}")) {
    node_t field = read_struct_field(ctx, tokens, &current, filename);
    if (node_is_error(field)) {
      allocator_free(allocator, &decorators);
      allocator_free(allocator, &name);
      allocator_free(allocator, &fields);
      return field;
    }
    if (!field) {
      break;
    }
    vec_push(fields, field);
    skip_whitespace(tokens, &current);
  }

  /* 5. Expect '}' */
  if (!_is_symbol(tokens, current, "}")) {
    goto onerror;
  }
  token_t close_brace = vec_get(tokens, current);
  current++;

  /* 6. Build location */
  location_t *end_loc = token_get_location(close_brace);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_statement_cunion_init_t init = {
      .location = loc,
      .parent = NULL,
      .name = name,
      .fields = fields,
      .decorators = decorators,
  };
  node = allocator_create(allocator, &g_cubec_statement_cunion_type, &init);
  *position = current;
  return &node->super;

onerror:
  allocator_free(allocator, &decorators);
  allocator_free(allocator, &fields);
  allocator_free(allocator, &name);
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

node_t create_statement_cunion(context_t ctx, location_t loc, const char *name,
                               vec_t fields, vec_t decorators) {
  allocator_t alloc = ctx->allocator;
  node_t name_node = create_literal_identifier(ctx, loc, name);
  cubec_statement_cunion_init_t init = {
      .location = loc,
      .parent = NULL,
      .name = name_node,
      .fields = fields,
      .decorators = decorators,
  };
  return (node_t)allocator_create(alloc, &g_cubec_statement_cunion_type, &init);
}

void emit_statement_cunion(emit_context_t ctx, node_t node) {
  cubec_statement_cunion_t stmt = (cubec_statement_cunion_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  if (stmt->decorators) {
    for (size_t i = 0; i < vec_get_size(stmt->decorators); i++) {
      recover_comments_to(ctx, ((node_t)vec_get(stmt->decorators, i))->location.begin.offset);
      emit_decorator(ctx, vec_get(stmt->decorators, i));
      emit_newline(ctx);
    }
  }
  emit_keyword(ctx, "cunion");
  emit_space(ctx);
  emit_expression(ctx, stmt->name);
  emit_space(ctx);
  emit_symbol(ctx, "{");
  if (vec_get_size(stmt->fields)) {
    emit_indent(ctx, +1);
    emit_newline(ctx);
    size_t count = vec_get_size(stmt->fields);
    for (size_t i = 0; i < count; i++) {
      recover_comments_to(ctx, ((node_t)vec_get(stmt->fields, i))->location.begin.offset);
      emit_struct_field(ctx, vec_get(stmt->fields, i));
      if (i + 1 < count) {
        emit_newline(ctx);
      }
    }
    emit_indent(ctx, -1);
    emit_newline(ctx);
  } else {
    emit_newline(ctx);
  }
  emit_symbol(ctx, "}");
}
