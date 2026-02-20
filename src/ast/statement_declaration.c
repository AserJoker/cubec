#include "ast/statement_declaration.h"
#include "ast/decorator.h"
#include "ast/literal_identifier.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/variable_declarator.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/location.h"
#include "core/position.h"
#include <corecrt_search.h>
static void
cubec_ast_statement_declaration_dispose(cubec_ast_statement_declaration_t self,
                                        cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->kind);
  cubec_allocator_free(allocator, self->declarations);
  cubec_allocator_free(allocator, self->decorators);
}
cubec_ast_statement_declaration_t
cubec_create_ast_statement_declaration(cubec_allocator_t allocator) {
  cubec_ast_statement_declaration_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_statement_declaration_t),
      (cubec_dispose_fn_t)cubec_ast_statement_declaration_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_STATEMENT_DECLARATION;
  self->kind = NULL;
  cubec_list_initialize_t initialize = {
      .autofree = true,
  };
  self->declarations = cubec_create_list(allocator, &initialize);
  self->decorators = cubec_create_list(allocator, &initialize);
  return self;
}
cubec_ast_node_t cubec_read_ast_statement_declaration(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_statement_declaration_t node =
      cubec_create_ast_statement_declaration(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  for (;;) {
    cubec_ast_node_t decorator =
        cubec_read_ast_decorator(allocator, &current, end);
    if (!decorator) {
      break;
    }
    if (decorator->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    cubec_list_append(node->decorators, allocator, decorator);
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
  }
  cubec_ast_node_t kind =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!kind) {
    goto onerror;
  }
  if (kind->type == CUBEC_NODE_TYPE_ERROR) {
    err = kind;
    goto onerror;
  }
  if (!cubec_location_is(kind->loc, "const") &&
      !cubec_location_is(kind->loc, "using") &&
      !cubec_location_is(kind->loc, "extern") &&
      !cubec_location_is(kind->loc, "let") &&
      !cubec_location_is(kind->loc, "comptime")) {
    cubec_allocator_free(allocator, kind);
    goto onerror;
  }
  node->kind = kind;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  for (;;) {
    cubec_ast_node_t item =
        cubec_read_ast_variable_declarator(allocator, &current, end);
    if (!item) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid or unexpected token");
      goto onerror;
    }
    if (item->type == CUBEC_NODE_TYPE_ERROR) {
      err = item;
      goto onerror;
    }
    cubec_list_append(node->declarations, allocator, item);
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    if (*current.offset == ';') {
      break;
    }
    if (*current.offset != ',') {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid or unexpected token");
      goto onerror;
    }
    current.offset++;
    current.column++;
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t token =
      cubec_read_ast_literal_symbol(allocator, &current, end);
  if (token && token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!token || !cubec_location_is(token->loc, ";")) {
    cubec_allocator_free(allocator, token);
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid expression statement, missing ';'");
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}