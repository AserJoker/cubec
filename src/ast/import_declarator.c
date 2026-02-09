#include "ast/import_declarator.h"
#include "ast/literal_identifier.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
#include <sys/types.h>

static void
cubec_ast_import_declarator_dispose(cubec_ast_import_declarator self,
                                    cubec_allocator_t allcator) {
  cubec_allocator_free(allcator, self->alias);
  cubec_allocator_free(allcator, self->identifier);
}

cubec_ast_import_declarator
cubec_create_ast_import_declarator(cubec_allocator_t allocator) {
  cubec_ast_import_declarator declarator = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_import_declarator),
      (cubec_dispose_fn_t)cubec_ast_import_declarator_dispose);
  cubec_ast_node_initialize(allocator, &declarator->super);
  declarator->super.type = CUBEC_NODE_TYPE_IMPORT_DECLARATOR;
  declarator->alias = NULL;
  declarator->identifier = NULL;
  return declarator;
}

static cubec_ast_node_t cubec_read_ast_import_declarator_alias(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_position_t current = *position;
  cubec_ast_node_t err = NULL;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  cubec_ast_node_t token =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!token || !cubec_location_is(token->loc, "as")) {
    cubec_allocator_free(allocator, token);
    return NULL;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  token = cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!token) {
    cubec_allocator_free(allocator, token);
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid import statement, missing alias");
    return err;
  }
  return token;
}

cubec_ast_node_t cubec_read_ast_import_declarator(cubec_allocator_t allocator,
                                                  cubec_position_t *position,
                                                  const char *end) {
  cubec_position_t current = *position;
  cubec_ast_import_declarator node = NULL;
  cubec_ast_node_t err = NULL;
  cubec_ast_node_t token =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!token) {
    return NULL;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  node = cubec_create_ast_import_declarator(allocator);
  node->identifier = token;
  token = cubec_read_ast_import_declarator_alias(allocator, &current, end);
  if (token) {
    if (token->type == CUBEC_NODE_TYPE_ERROR) {
      err = token;
      goto onerror;
    } else {
      node->alias = token;
    }
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}

cubec_ast_node_t cubec_read_ast_import_namespace(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end) {
  cubec_position_t current = *position;
  cubec_ast_import_declarator node = NULL;
  cubec_ast_node_t err = NULL;
  cubec_ast_node_t token =
      cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!token) {
    return NULL;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    return err;
  }
  if (!cubec_location_is(token->loc, "*")) {
    cubec_allocator_free(allocator, token);
    return NULL;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  token = cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!token) {
    cubec_allocator_free(allocator, token);
    err =
        cubec_create_ast_error(allocator, *position, current,
                               "Invalid import namespace, missing token 'as'");
    return err;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    return err;
  }
  if (cubec_location_is(token->loc, "as")) {
    cubec_allocator_free(allocator, token);
    err =
        cubec_create_ast_error(allocator, *position, current,
                               "Invalid import namespace, missing token 'as'");
    return err;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  token = cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!token) {
    cubec_allocator_free(allocator, token);
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid import namespace, missing alias");
    return err;
  }
  return token;
}