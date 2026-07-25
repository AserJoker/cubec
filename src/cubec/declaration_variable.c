#include "cubec/declaration_variable.h"
#include "cubec/ast_factory_internal.h"
#include "core/allocator.h"
#include "core/token.h"
#include "cubec/ast_factory.h"
#include "cubec/expression.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "engine/context.h"

static void _cubec_declaration_variable_init(cubec_declaration_variable_t self,
                                             allocator_t allocator,
                                             cubec_declaration_variable_init_t *init) {
  if (!init) return;
  cubec_declaration_init_t super_init = {
      .kind = CUBEC_NODE_DECLARATION_VARIABLE,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_cubec_declaration_type.init(&self->super, allocator, &super_init);
  self->identifier = init->identifier;
  self->type = init->type;
  self->expression = init->expression;
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
  g_cubec_declaration_type.clone(&self->super, allocator, &another->super);
  self->identifier = value_clone(allocator, another->identifier);
  self->type = value_clone(allocator, another->type);
  self->expression = value_clone(allocator, another->expression);
  return;

cleanup:
  allocator_free(allocator, &self->identifier);
  allocator_free(allocator, &self->type);
  allocator_free(allocator, &self->expression);
}

static void _cubec_declaration_variable_move(cubec_declaration_variable_t self,
                                             allocator_t allocator,
                                             cubec_declaration_variable_t another) {
  g_cubec_declaration_type.move(&self->super, allocator, &another->super);
  self->identifier = value_move(allocator, another->identifier);
  self->type = value_move(allocator, another->type);
  self->expression = value_move(allocator, another->expression);
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

node_t read_declaration_variable(context_t ctx, vec_t tokens,
                                 size_t *position, const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  cubec_declaration_variable_t node = NULL;
  node_t identifier = NULL;
  node_t type = NULL;
  node_t expression = NULL;
  location_t start_location = {0};

  /* Check for identifier first to get start location */
  token_t ident_token = vec_get(tokens, current);
  if (token_get_kind(ident_token) != CUBEC_TOKEN_IDENTIFIER) {
    return NULL;
  }
  start_location = *token_get_location(ident_token);
  start_location.filename = filename;

  /* Parse identifier using read_literal_identifier */
  identifier = read_literal_identifier(ctx, tokens, &current, filename);
  if (!identifier) {
    return NULL;
  }

  skip_whitespace(tokens, &current);

  /* Check for optional type annotation ': <type>' */
  token_t colon_token = vec_get(tokens, current);
  if (token_is(colon_token, CUBEC_TOKEN_SYMBOL, ":")) {
    current++;
    skip_whitespace(tokens, &current);

    /* Parse the type using read_expression_base (no comma/assignment —
     * read_expression_type includes assignment which would consume '=' as
     * part of the type, breaking the type/init split) */
    type = read_expression_base(ctx, tokens, &current, filename);
    if (!type) {
      goto cleanup_node;
    }

    skip_whitespace(tokens, &current);
  }

  /* Expect '=' (optional — absent for extern/builtin declarations) */
  token_t eq_token = vec_get(tokens, current);
  if (token_is(eq_token, CUBEC_TOKEN_SYMBOL, "=")) {
    current++;

    skip_whitespace(tokens, &current);

    /* Parse the initializer expression using read_expression_base
     * (no comma — comma in var init would conflict with comma-separated
     * declarator lists, and assignment is already handled by the '=' above) */
    expression = read_expression_base(ctx, tokens, &current, filename);
    if (!expression) {
      goto cleanup_node;
    }
  }

  /* Create the variable declarator node */
  node = allocator_create(allocator, &g_cubec_declaration_variable_type,
                          &(cubec_declaration_variable_init_t){
                              .identifier = identifier,
                              .type = type,
                              .expression = expression,
                          });
  if (!node) goto cleanup_node;

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

/* --------------------------------------------------------------------------
 *  Factory: cubec_ast_create_variable_decl
 * -------------------------------------------------------------------------- */

node_t cubec_ast_create_variable_decl(context_t ctx, location_t loc,
                                      node_t identifier, node_t type,
                                      node_t expression) {
  allocator_t alloc = ctx->allocator;
      cubec_declaration_variable_init_t init = {
      .location = loc, .parent = NULL, .identifier = identifier,
      .type = type, .expression = expression};
  return (node_t)allocator_create(alloc, &g_cubec_declaration_variable_type,
                                  &init);
}