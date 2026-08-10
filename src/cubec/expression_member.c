#include "core/emit_context.h"
#include "core/token_writer.h"
#include "cubec/expression_member.h"
#include "core/token.h"
#include "cubec/token.h"

static void
_cubec_expression_member_init(cubec_expression_member_t self,
                              allocator_t allocator,
                              cubec_expression_member_init_t *init) {
  if (!init)
    return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_MEMBER,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_class.init(&self->super, allocator, &super_init);
  self->host = init->host;
  self->field = init->field;
}

static void _cubec_expression_member_dispose(cubec_expression_member_t self,
                                             allocator_t allocator) {
  allocator_free(allocator, &self->host);
  allocator_free(allocator, &self->field);
  g_cubec_expression_class.dispose(&self->super, allocator);
}

static void _cubec_expression_member_clone(cubec_expression_member_t self,
                                           allocator_t allocator,
                                           cubec_expression_member_t another) {
  g_cubec_expression_class.clone(&self->super, allocator, &another->super);
  self->host = alloc_clone(allocator, another->host);
  self->field =
      (cubec_literal_identifier_t)alloc_clone(allocator, another->field);
  return;

cleanup:
  allocator_free(allocator, &self->field);
  allocator_free(allocator, &self->host);
}

static void _cubec_expression_member_move(cubec_expression_member_t self,
                                          allocator_t allocator,
                                          cubec_expression_member_t another) {
  g_cubec_expression_class.move(&self->super, allocator, &another->super);
  self->host = alloc_move(allocator, another->host);
  self->field =
      (cubec_literal_identifier_t)alloc_move(allocator, another->field);
  return;

cleanup:
  allocator_free(allocator, &self->field);
  allocator_free(allocator, &self->host);
}

class_t g_cubec_expression_member_class = {
    .name = "cubec.cubec.expression_member",
    .size = sizeof(struct _cubec_expression_member_t),
    .init = (class_init_fn_t)_cubec_expression_member_init,
    .dispose = (class_dispose_fn_t)_cubec_expression_member_dispose,
    .clone = (class_clone_fn_t)_cubec_expression_member_clone,
    .move = (class_move_fn_t)_cubec_expression_member_move,
};

node_t read_expression_member(context_t ctx, vec_t tokens, size_t *position,
                              const char *filename, node_t host) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  cubec_expression_member_t node = NULL;
  cubec_literal_identifier_t field = NULL;

  /* Expect '.' (caller ensures whitespace already skipped) */
  token_t dot_token = vec_get(tokens, current);
  if (!token_is(dot_token, CUBEC_TOKEN_SYMBOL, ".")) {
    return NULL;
  }
  current++;

  /* Expect identifier after '.' */
  skip_whitespace(tokens, &current);
  node_t field_node = read_literal_identifier(ctx, tokens, &current, filename);
  if (!field_node) {
    return NULL;
  }
  field = (cubec_literal_identifier_t)field_node;

  node = allocator_create(allocator, &g_cubec_expression_member_class,
                          &(cubec_expression_member_init_t){
                              .host = host,
                              .field = field,
                          });
  location_t *loc = token_get_location(dot_token);
  node->super.super.location = *loc;
  node->super.super.location.filename = filename;

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &field);
  allocator_free(allocator, &node);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Factory: create_expression_member
 * -------------------------------------------------------------------------- */

node_t create_expression_member(context_t ctx, location_t loc, node_t host,
                                const char *field) {
  allocator_t alloc = ctx->allocator;
  cubec_expression_member_init_t init = {
      .location = loc,
      .parent = NULL,
      .host = host,
      .field = (cubec_literal_identifier_t)create_literal_identifier(ctx, loc,
                                                                     field),
  };
  return (node_t)allocator_create(alloc, &g_cubec_expression_member_class,
                                  &init);
}

void emit_expression_member(emit_context_t ctx, node_t node) {
  cubec_expression_member_t member = (cubec_expression_member_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_expression(ctx, member->host);
  emit_symbol(ctx, ".");
  emit_expression(ctx, (node_t)member->field);
}
