#include "ast/literal_numeric.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
static void cubec_ast_literal_numeric_dispose(cubec_ast_literal_numeric_t self,
                                              cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->flag);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_literal_numeric_t
cubec_create_ast_literal_numeric(cubec_allocator_t allocator) {
  cubec_ast_literal_numeric_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_literal_numeric_t),
      (cubec_dispose_fn_t)cubec_ast_literal_numeric_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_LITERAL_NUMERIC;
  self->is_exp = false;
  self->is_float = false;
  self->flag = NULL;
  return self;
}

cubec_ast_node_t cubec_read_ast_literal_numeric(cubec_allocator_t allocator,
                                                cubec_position_t *position,
                                                const char *end) {
  cubec_ast_literal_numeric_t node =
      cubec_create_ast_literal_numeric(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  if (*current.offset == '0' &&
      (*(current.offset + 1) == 'x' || *(current.offset + 1) == 'X')) {
    current.offset += 2;
    current.column += 2;
    if (!(*current.offset >= '0' && *current.offset <= '9') ||
        (*current.offset >= 'a' && *current.offset <= 'f') ||
        (*current.offset >= 'A' && *current.offset <= 'F')) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid or unexpected token");
      goto onerror;
    }
    while ((*current.offset >= '0' && *current.offset <= '9') ||
           (*current.offset >= 'a' && *current.offset <= 'f') ||
           (*current.offset >= 'A' && *current.offset <= 'F') ||
           *current.offset == '_') {
      current.offset++;
      current.column++;
    }
    if (*(current.offset - 1) == '_') {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid or unexpected token");
      goto onerror;
    }
    cubec_ast_node_t token =
        cubec_read_ast_literal_identifier(allocator, &current, end);
    if (token) {
      if (token->type == CUBEC_NODE_TYPE_ERROR) {
        err = token;
        goto onerror;
      }
      if (!cubec_location_is(token->loc, "u") &&
          !cubec_location_is(token->loc, "l") &&
          !cubec_location_is(token->loc, "ul")) {
        current = token->loc.begin;
        cubec_allocator_free(allocator, token);
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid or unexpected token");
        goto onerror;
      }
      node->flag = token;
    }
  } else if (*current.offset == '0' &&
             (*(current.offset + 1) == 'o' || *(current.offset + 1) == 'O')) {
    current.offset += 2;
    current.column += 2;
    if (!(*current.offset >= '0' && *current.offset <= '7')) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid or unexpected token");
      goto onerror;
    }
    while ((*current.offset >= '0' && *current.offset <= '7') ||
           *current.offset == '_') {
      current.offset++;
      current.column++;
    }
    if (*(current.offset - 1) == '_') {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid or unexpected token");
      goto onerror;
    }
    cubec_ast_node_t token =
        cubec_read_ast_literal_identifier(allocator, &current, end);
    if (token) {
      if (token->type == CUBEC_NODE_TYPE_ERROR) {
        err = token;
        goto onerror;
      }
      if (!cubec_location_is(token->loc, "u") &&
          !cubec_location_is(token->loc, "l") &&
          !cubec_location_is(token->loc, "ul")) {
        current = token->loc.begin;
        cubec_allocator_free(allocator, token);
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid or unexpected token");
        goto onerror;
      }
      node->flag = token;
    }
  } else if (*current.offset == '0' &&
             (*(current.offset + 1) == 'b' || *(current.offset + 1) == 'B')) {
    current.offset += 2;
    current.column += 2;
    if (!(*current.offset >= '0' && *current.offset <= '1')) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid or unexpected token");
      goto onerror;
    }
    while ((*current.offset >= '0' && *current.offset <= '1') ||
           *current.offset == '_') {
      current.offset++;
      current.column++;
    }
    if (*(current.offset - 1) == '_') {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid or unexpected token");
      goto onerror;
    }
    cubec_ast_node_t token =
        cubec_read_ast_literal_identifier(allocator, &current, end);
    if (token) {
      if (token->type == CUBEC_NODE_TYPE_ERROR) {
        err = token;
        goto onerror;
      }
      if (!cubec_location_is(token->loc, "u") &&
          !cubec_location_is(token->loc, "l") &&
          !cubec_location_is(token->loc, "ul")) {
        current = token->loc.begin;
        cubec_allocator_free(allocator, token);
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid or unexpected token");
        goto onerror;
      }
      node->flag = token;
    }
  } else if (*current.offset >= '0' && *current.offset <= '9' ||
             *current.offset == '.') {
    if (*current.offset >= '0' && *current.offset <= '9') {
      while (*current.offset >= '0' && *current.offset <= '9' ||
             *current.offset == '_') {
        current.offset++;
        current.column++;
      }
      if (*(current.offset - 1) == '_') {
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid or unexpected token");
        goto onerror;
      }
    }
    if (*current.offset == '.') {
      node->is_float = true;
      current.offset++;
      current.column++;
      if ((*current.offset < '0' || *current.offset > '9') &&
          *position->offset == '.') {
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid or unexpected token");
        goto onerror;
      }
      while (*current.offset >= '0' && *current.offset <= '9' ||
             *current.offset == '_') {
        current.offset++;
        current.column++;
      }
      if (*(current.offset - 1) == '_') {
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid or unexpected token");
        goto onerror;
      }
    }
    if (*current.offset == 'e' || *current.offset == 'E') {
      current.offset++;
      current.column++;
      node->is_exp = true;
      if (*current.offset < '0' || *current.offset > '9') {
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid or unexpected token");
        goto onerror;
      }
      while (*current.offset >= '0' && *current.offset <= '9' ||
             *current.offset == '_') {
        current.offset++;
        current.column++;
      }
      if (*(current.offset - 1) == '_') {
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid or unexpected token");
        goto onerror;
      }
    }
    cubec_ast_node_t token =
        cubec_read_ast_literal_identifier(allocator, &current, end);
    if (token) {
      if (token->type == CUBEC_NODE_TYPE_ERROR) {
        err = token;
        goto onerror;
      }
      if (!node->is_float) {
        if (!cubec_location_is(token->loc, "u") &&
            !cubec_location_is(token->loc, "l") &&
            !cubec_location_is(token->loc, "ul")) {
          current = token->loc.begin;
          cubec_allocator_free(allocator, token);
          err = cubec_create_ast_error(allocator, *position, current,
                                       "Invalid or unexpected token");
          goto onerror;
        }
      } else {
        if (!cubec_location_is(token->loc, "f")) {
          current = token->loc.begin;
          cubec_allocator_free(allocator, token);
          err = cubec_create_ast_error(allocator, *position, current,
                                       "Invalid or unexpected token");
          goto onerror;
        }
      }
      node->flag = token;
    }
  } else {
    goto onerror;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}