#include "ast/enum_declarator.h"
#include "ast/decorator.h"
#include "ast/enum_field.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/type.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/location.h"
#include "core/position.h"
static void cubec_ast_enum_declarator_dispose(cubec_ast_enum_declarator_t self,
                                              cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->decorators);
  cubec_allocator_free(allocator, self->identifier);
  cubec_allocator_free(allocator, self->type);
  cubec_allocator_free(allocator, self->fields);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_enum_declarator_t
cubec_create_ast_enum_declarator(cubec_allocator_t allocator) {
  cubec_ast_enum_declarator_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_enum_declarator_t),
      (cubec_dispose_fn_t)cubec_ast_enum_declarator_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_ENUM_DECLARATOR;
  self->identifier = NULL;
  self->type = NULL;
  cubec_list_initialize_t initialize = {
      .autofree = true,
      .compare = NULL,
  };
  self->decorators = cubec_create_list(allocator, &initialize);
  self->fields = cubec_create_list(allocator, &initialize);
  return self;
}
cubec_ast_node_t cubec_read_ast_enum_declarator(cubec_allocator_t allocator,
                                                cubec_position_t *position,
                                                const char *end) {
  cubec_ast_enum_declarator_t node =
      cubec_create_ast_enum_declarator(allocator);
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
  cubec_ast_node_t token =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!token) {
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!cubec_location_is(token->loc, "enum")) {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t identifier =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (identifier) {
    if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
      err = identifier;
      goto onerror;
    }
    node->identifier = identifier;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset == ':') {
    current.offset++;
    current.column++;
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
    cubec_ast_node_t type = cubec_read_ast_type(allocator, &current, end);
    if (!type) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid enum declarator, missing 'type'");
      goto onerror;
    }
    if (type->type == CUBEC_NODE_TYPE_ERROR) {
      err = type;
      goto onerror;
    }
    node->type = type;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '{') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid enum declarator, missing '{'");
    goto onerror;
  }
  current.column++;
  current.offset++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '}') {
    for (;;) {
      cubec_ast_node_t item =
          cubec_read_ast_enum_field(allocator, &current, end);
      if (!item) {
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid enum declarator");
        goto onerror;
      }
      if (item->type == CUBEC_NODE_TYPE_ERROR) {
        err = item;
        goto onerror;
      }
      cubec_list_append(node->fields, allocator, item);
      err = cubec_ast_skip_all(allocator, &current, end);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
      if (*current.offset == '}') {
        break;
      }
      if (*current.offset != ',') {
        err = cubec_create_ast_error(allocator, *position, current,
                                     "Invalid enum declarator, missing ','");
        goto onerror;
      }
      current.offset++;
      current.column++;
      err = cubec_ast_skip_all(allocator, &current, end);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
      if (*current.offset == '}') {
        break;
      }
    }
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '}') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid enum declarator, missing '}'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}