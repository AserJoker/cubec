#include "cubec/expression_namespace_access.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/token.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"

static void _cubec_expression_namespace_access_init(
    cubec_expression_namespace_access_t self, allocator_t allocator,
    cubec_expression_namespace_access_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  TRY_VOID_LOCAL(onerror,
                 g_cubec_expression_type.init(&self->super, allocator, &super_init));
  self->host = init->host;
  self->field = init->field;
onerror:
  return;
}

static void _cubec_expression_namespace_access_dispose(
    cubec_expression_namespace_access_t self, allocator_t allocator) {
  allocator_free(allocator, &self->host);
  allocator_free(allocator, &self->field);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_namespace_access_clone(
    cubec_expression_namespace_access_t self, allocator_t allocator,
    cubec_expression_namespace_access_t another) {
  TRY_VOID_LOCAL(cleanup,
                 g_cubec_expression_type.clone(&self->super, allocator, &another->super));
  self->host = TRY_LOCAL(cleanup, value_clone(allocator, another->host));
  self->field = (cubec_literal_identifier_t)TRY_LOCAL(
      cleanup, value_clone(allocator, another->field));
  return;

cleanup:
  allocator_free(allocator, &self->field);
  allocator_free(allocator, &self->host);
}

static void _cubec_expression_namespace_access_move(
    cubec_expression_namespace_access_t self, allocator_t allocator,
    cubec_expression_namespace_access_t another) {
  TRY_VOID_LOCAL(cleanup,
                 g_cubec_expression_type.move(&self->super, allocator, &another->super));
  self->host = TRY_LOCAL(cleanup, value_move(allocator, another->host));
  self->field = (cubec_literal_identifier_t)TRY_LOCAL(
      cleanup, value_move(allocator, another->field));
  return;

cleanup:
  allocator_free(allocator, &self->field);
  allocator_free(allocator, &self->host);
}

type_t g_cubec_expression_namespace_access_type = {
    .name = "cubec.cubec.expression_namespace_access",
    .size = sizeof(struct _cubec_expression_namespace_access_t),
    .init = (type_init_fn_t)_cubec_expression_namespace_access_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_namespace_access_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_namespace_access_clone,
    .move = (type_move_fn_t)_cubec_expression_namespace_access_move,
};

node_t read_expression_namespace_access(allocator_t allocator, vec_t tokens,
                                        size_t *position, const char *filename,
                                        node_t host) {
  size_t current = *position;
  cubec_expression_namespace_access_t node = NULL;
  cubec_literal_identifier_t field = NULL;

  /* Expect '::' */
  token_t colon_colon_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (!token_is(colon_colon_token, CUBEC_TOKEN_SYMBOL, "::")) {
    return NULL;
  }
  current++;

  /* Expect identifier after '::' */
  skip_whitespace(tokens, &current);
  node_t field_node = TRY_LOCAL(
      onerror, read_literal_identifier(allocator, tokens, &current, filename));
  if (!field_node) {
    return NULL;
  }
  field = (cubec_literal_identifier_t)field_node;

  node = TRY_LOCAL(
      onerror,
      allocator_create(allocator, &g_cubec_expression_namespace_access_type,
                       &(cubec_expression_namespace_access_init_t){
                           .host = host,
                           .field = field,
                       }));
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
