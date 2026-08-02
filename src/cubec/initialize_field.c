#include "core/token.h"
#include "cubec/initialize_field.h"
#include "cubec/literal_identifier.h"
#include "cubec/node_error.h"
#include "cubec/token.h"

static void _cubec_initialize_field_init(cubec_initialize_field_t self,
                                         allocator_t allocator,
                                         cubec_initialize_field_init_t *init) {
  if (!init)
    return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_INITIALIZE_FIELD,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_type.init(&self->super, allocator, &super_init);
  self->field = init->field;
  self->value = init->value;
}

static void _cubec_initialize_field_dispose(cubec_initialize_field_t self,
                                            allocator_t allocator) {
  allocator_free(allocator, &self->field);
  allocator_free(allocator, &self->value);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_initialize_field_clone(cubec_initialize_field_t self,
                                          allocator_t allocator,
                                          cubec_initialize_field_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->field =
      (cubec_literal_identifier_t)value_clone(allocator, another->field);
  self->value = value_clone(allocator, another->value);
  return;

cleanup:
  allocator_free(allocator, &self->value);
  allocator_free(allocator, &self->field);
}

static void _cubec_initialize_field_move(cubec_initialize_field_t self,
                                         allocator_t allocator,
                                         cubec_initialize_field_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->field =
      (cubec_literal_identifier_t)value_move(allocator, another->field);
  self->value = value_move(allocator, another->value);
  return;

cleanup:
  allocator_free(allocator, &self->value);
  allocator_free(allocator, &self->field);
}

type_t g_cubec_initialize_field_type = {
    .name = "cubec.cubec.expression_initialize_field",
    .size = sizeof(struct _cubec_initialize_field_t),
    .init = (type_init_fn_t)_cubec_initialize_field_init,
    .dispose = (type_dispose_fn_t)_cubec_initialize_field_dispose,
    .clone = (type_clone_fn_t)_cubec_initialize_field_clone,
    .move = (type_move_fn_t)_cubec_initialize_field_move,
};

node_t read_initialize_field(context_t ctx, vec_t tokens, size_t *position,
                             const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  cubec_initialize_field_t node = NULL;
  cubec_literal_identifier_t field = NULL;
  node_t value = NULL;
  token_t dot_token = NULL;
  location_t dot_location = {0};

  /* Expect '.' (caller ensures whitespace already skipped) */
  dot_token = vec_get(tokens, current);
  if (!token_is(dot_token, CUBEC_TOKEN_SYMBOL, ".")) {
    return NULL;
  }
  dot_location = *token_get_location(dot_token);
  current++;

  /* Expect identifier after '.' */
  skip_whitespace(tokens, &current);
  node_t field_node = read_literal_identifier(ctx, tokens, &current, filename);
  if (!field_node) {
    goto onerror;
  }
  field = (cubec_literal_identifier_t)field_node;

  /* Expect '=' */
  skip_whitespace(tokens, &current);
  token_t eq_token = vec_get(tokens, current);
  if (!token_is(eq_token, CUBEC_TOKEN_SYMBOL, "=")) {
    allocator_free(allocator, &field);
    return NULL;
  }
  current++;

  /* Parse the value expression */
  skip_whitespace(tokens, &current);
  value = read_expression_base(ctx, tokens, &current, filename);
  if (!value) {
    goto onerror;
  }

  node = allocator_create(allocator, &g_cubec_initialize_field_type,
                          &(cubec_initialize_field_init_t){
                              .field = field,
                              .value = value,
                          });
  node->super.super.location = dot_location;
  node->super.super.location.filename = filename;

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &value);
  allocator_free(allocator, &field);
  allocator_free(allocator, &node);
  return create_error(ctx, dot_location);
}

/* --------------------------------------------------------------------------
 *  Factory: create_initialize_field
 * -------------------------------------------------------------------------- */

node_t create_initialize_field(context_t ctx, location_t loc, const char *name,
                               node_t value) {
  allocator_t alloc = ctx->allocator;
  cubec_initialize_field_init_t init = {
      .location = loc,
      .parent = NULL,
      .field =
          (cubec_literal_identifier_t)create_literal_identifier(ctx, loc, name),
      .value = value,
  };
  return (node_t)allocator_create(alloc, &g_cubec_initialize_field_type, &init);
}
