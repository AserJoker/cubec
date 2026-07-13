#include "cubec/declaration_variable.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"

static void _cubec_declaration_variable_init(cubec_declaration_variable_t self,
                                             allocator_t allocator,
                                             cubec_declaration_variable_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_declaration_init_t super_init = {
      .kind = CUBEC_NODE_DECLARATION_VARIABLE,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_cubec_declaration_type.init(&self->super, allocator, &super_init));
  self->identifier = init->identifier;
  self->type = init->type;
  self->expression = init->expression;
onerror:
  return;
}

static void _cubec_declaration_variable_dispose(cubec_declaration_variable_t self,
                                                allocator_t allocator) {
  allocator_free(allocator, &self->identifier);
  allocator_free(allocator, &self->type);
  allocator_free(allocator, &self->expression);
  g_cubec_declaration_type.dispose(&self->super, allocator);
}

static void _cubec_declaration_variable_clone(cubec_declaration_variable_t self,
                                              allocator_t allocator,
                                              cubec_declaration_variable_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_declaration_type.clone(&self->super, allocator, &another->super));
  self->identifier = TRY_LOCAL(cleanup, value_clone(allocator, another->identifier));
  self->type = TRY_LOCAL(cleanup, value_clone(allocator, another->type));
  self->expression = TRY_LOCAL(cleanup, value_clone(allocator, another->expression));
  return;

cleanup:
  allocator_free(allocator, &self->identifier);
  allocator_free(allocator, &self->type);
  allocator_free(allocator, &self->expression);
}

static void _cubec_declaration_variable_move(cubec_declaration_variable_t self,
                                             allocator_t allocator,
                                             cubec_declaration_variable_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_declaration_type.move(&self->super, allocator, &another->super));
  self->identifier = TRY_LOCAL(cleanup, value_move(allocator, another->identifier));
  self->type = TRY_LOCAL(cleanup, value_move(allocator, another->type));
  self->expression = TRY_LOCAL(cleanup, value_move(allocator, another->expression));
  return;

cleanup:
  allocator_free(allocator, &self->identifier);
  allocator_free(allocator, &self->type);
  allocator_free(allocator, &self->expression);
}

type_t g_cubec_declaration_variable_type = {
    .name = "cubec.cubec.declaration_variable",
    .size = sizeof(struct _cubec_declaration_variable_t),
    .init = (type_init_fn_t)_cubec_declaration_variable_init,
    .dispose = (type_dispose_fn_t)_cubec_declaration_variable_dispose,
    .clone = (type_clone_fn_t)_cubec_declaration_variable_clone,
    .move = (type_move_fn_t)_cubec_declaration_variable_move,
};

node_t read_declaration_variable(allocator_t allocator, vec_t tokens,
                                 size_t *position, const char *filename) {
  size_t current = *position;
  cubec_declaration_variable_t node = NULL;
  node_t identifier = NULL;
  node_t type = NULL;
  node_t expression = NULL;
  location_t start_location = {0};

  /* Check for identifier first to get start location */
  token_t ident_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (token_get_kind(ident_token) != CUBEC_TOKEN_IDENTIFIER) {
    return NULL;
  }
  start_location = *token_get_location(ident_token);
  start_location.filename = filename;

  /* Parse identifier using read_literal_identifier */
  identifier = TRY_LOCAL(onerror, read_literal_identifier(allocator, tokens, &current, filename));
  if (!identifier) {
    return NULL;
  }

  skip_whitespace(tokens, &current);

  /* Check for optional type annotation ': <type>' */
  token_t colon_token = TRY_LOCAL(cleanup_node, vec_get(tokens, current));
  if (token_is(colon_token, CUBEC_TOKEN_SYMBOL, ":")) {
    current++;
    skip_whitespace(tokens, &current);

    /* Parse the type using read_expression_type */
    type = TRY_LOCAL(cleanup_node, read_expression_type(allocator, tokens, &current, filename));
    if (!type) {
      THROW_LOCAL(cleanup_node, "expected type after ':'");
    }

    skip_whitespace(tokens, &current);
  }

  /* Expect '=' (optional — absent for extern/builtin declarations) */
  token_t eq_token = TRY_LOCAL(cleanup_node, vec_get(tokens, current));
  if (token_is(eq_token, CUBEC_TOKEN_SYMBOL, "=")) {
    current++;

    skip_whitespace(tokens, &current);

    /* Parse the initializer expression using read_expression */
    expression = TRY_LOCAL(cleanup_node, read_expression(allocator, tokens, &current, filename));
    if (!expression) {
      THROW_LOCAL(cleanup_node, "expected expression after '='");
    }
  }

  /* Create the variable declarator node */
  node = TRY_LOCAL(cleanup_node, allocator_create(allocator, &g_cubec_declaration_variable_type,
                          &(cubec_declaration_variable_init_t){
                              .identifier = identifier,
                              .type = type,
                              .expression = expression,
                          }));

  /* Set location from start to end of expression (or type/identifier if no expression) */
  node->super.super.super.location = start_location;
  if (expression) {
    node->super.super.super.location.end = expression->location.end;
  } else if (type) {
    node->super.super.super.location.end = type->location.end;
  } else {
    node->super.super.super.location.end = identifier->location.end;
  }

  *position = current;
  return (node_t)node;

cleanup_node:
  allocator_free(allocator, &expression);
  allocator_free(allocator, &type);
  allocator_free(allocator, &identifier);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &expression);
  allocator_free(allocator, &type);
  allocator_free(allocator, &identifier);
  allocator_free(allocator, &node);
  return NULL;
}