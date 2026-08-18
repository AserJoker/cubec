#include "core/emit_context.h"
#include "core/token_writer.h"
#include "cubec/expression_namespace_access.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/token.h"

static void _cubec_expression_namespace_access_init(
    cubec_expression_namespace_access_t self, allocator_t allocator,
    cubec_expression_namespace_access_init_t *init) {
  if (!init)
    return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_class.init(&self->super, allocator, &super_init);
  self->host = init->host;
  self->field = init->field;
}

static void _cubec_expression_namespace_access_dispose(
    cubec_expression_namespace_access_t self, allocator_t allocator) {
  allocator_free(allocator, &self->host);
  allocator_free(allocator, &self->field);
  g_cubec_expression_class.dispose(&self->super, allocator);
}

static void _cubec_expression_namespace_access_clone(
    cubec_expression_namespace_access_t self, allocator_t allocator,
    cubec_expression_namespace_access_t another) {
  g_cubec_expression_class.clone(&self->super, allocator, &another->super);
  self->host = alloc_clone(allocator, another->host);
  self->field =
      (cubec_literal_identifier_t)alloc_clone(allocator, another->field);
  return;

cleanup:
  allocator_free(allocator, &self->field);
  allocator_free(allocator, &self->host);
}

static void _cubec_expression_namespace_access_move(
    cubec_expression_namespace_access_t self, allocator_t allocator,
    cubec_expression_namespace_access_t another) {
  g_cubec_expression_class.move(&self->super, allocator, &another->super);
  self->host = alloc_move(allocator, another->host);
  self->field =
      (cubec_literal_identifier_t)alloc_move(allocator, another->field);
  return;

cleanup:
  allocator_free(allocator, &self->field);
  allocator_free(allocator, &self->host);
}

class_t g_cubec_expression_namespace_access_class = {
    .name = "cubec.cubec.expression_namespace_access",
    .size = sizeof(struct _cubec_expression_namespace_access_t),
    .init = (class_init_fn_t)_cubec_expression_namespace_access_init,
    .dispose = (class_dispose_fn_t)_cubec_expression_namespace_access_dispose,
    .clone = (class_clone_fn_t)_cubec_expression_namespace_access_clone,
    .move = (class_move_fn_t)_cubec_expression_namespace_access_move,
};

node_t read_expression_namespace_access(vm_t vm, vec_t tokens,
                                        size_t *position, const char *filename,
                                        node_t host) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;
  cubec_expression_namespace_access_t node = NULL;
  cubec_literal_identifier_t field = NULL;

  /* Expect '::' */
  token_t colon_colon_token = vec_get(tokens, current);
  if (!token_is(colon_colon_token, CUBEC_TOKEN_SYMBOL, "::")) {
    return NULL;
  }
  current++;

  /* Expect identifier after '::' */
  skip_whitespace(tokens, &current);
  node_t field_node = read_literal_identifier(vm, tokens, &current, filename);
  if (!field_node) {
    return NULL;
  }
  field = (cubec_literal_identifier_t)field_node;

  node = allocator_create(allocator, &g_cubec_expression_namespace_access_class,
                          &(cubec_expression_namespace_access_init_t){
                              .host = host,
                              .field = field,
                          });
  location_t *loc = token_get_location(colon_colon_token);
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
 *  Factory: create_expression_namespace_access
 * -------------------------------------------------------------------------- */

node_t create_expression_namespace_access(vm_t vm, location_t loc,
                                          node_t host, const char *field) {
  allocator_t alloc = vm_get_allocator(vm);
  cubec_expression_namespace_access_init_t init = {
      .location = loc,
      .parent = NULL,
      .host = host,
      .field = (cubec_literal_identifier_t)create_literal_identifier(vm, loc,
                                                                     field),
  };
  return (node_t)allocator_create(
      alloc, &g_cubec_expression_namespace_access_class, &init);
}

/* --------------------------------------------------------------------------
 *  Writer: write_expression_namespace_access
 * -------------------------------------------------------------------------- */

void emit_expression_namespace_access(emit_context_t ctx, node_t node) {
  cubec_expression_namespace_access_t access = (cubec_expression_namespace_access_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_expression(ctx, access->host);
  emit_symbol(ctx, "::");
  emit_expression(ctx, (node_t)access->field);
}
