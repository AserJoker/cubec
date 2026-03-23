#include "ast/struct_declarator.h"
#include "ast/decorator.h"
#include "ast/enum_declarator.h"
#include "ast/function_declarator.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement_declaration.h"
#include "ast/struct_field.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
static void
cubec_ast_struct_declarator_dispose(cubec_ast_struct_declarator_t self,
                                    cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->methods);
  cubec_allocator_free(allocator, self->fields);
  cubec_allocator_free(allocator, self->attributes);
  cubec_allocator_free(allocator, self->decorators);
  cubec_allocator_free(allocator, self->identifier);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_struct_declarator_t
cubec_create_ast_struct_declarator(cubec_allocator_t allocator) {
  cubec_ast_struct_declarator_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_struct_declarator_t),
      (cubec_dispose_fn_t)cubec_ast_struct_declarator_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_STRUCT_DECLARATOR;
  cubec_ast_set_field(self, allocator, identifier);
  cubec_ast_set_field(self, allocator, fields);
  cubec_ast_set_field(self, allocator, methods);
  cubec_ast_set_field(self, allocator, attributes);
  cubec_ast_set_field(self, allocator, decorators);
  self->fields = cubec_create_ast_list_node(allocator);
  self->methods = cubec_create_ast_list_node(allocator);
  self->decorators = cubec_create_ast_list_node(allocator);
  self->attributes = cubec_create_ast_list_node(allocator);
  return self;
}
cubec_ast_node_t cubec_read_ast_struct_declarator(cubec_allocator_t allocator,
                                                  cubec_position_t *position,
                                                  const char *end) {
  cubec_ast_struct_declarator_t node =
      cubec_create_ast_struct_declarator(allocator);
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
    cubec_ast_list_node_append(node->decorators, allocator, decorator);
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
  if (!cubec_location_is(token->loc, "struct") &&
      !cubec_location_is(token->loc, "union")) {
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
  if (*current.offset != '{') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid struct declarator, missing '{'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '}') {
    for (;;) {
      cubec_ast_node_t item =
          cubec_read_ast_struct_declarator(allocator, &current, end);
      if (item) {
        if (item->type == CUBEC_NODE_TYPE_ERROR) {
          err = item;
          goto onerror;
        }
        cubec_ast_list_node_append(node->attributes, allocator, item);
        goto next;
      }
      item = cubec_read_ast_enum_declarator(allocator, &current, end);
      if (item) {
        if (item->type == CUBEC_NODE_TYPE_ERROR) {
          err = item;
          goto onerror;
        }
        cubec_ast_list_node_append(node->attributes, allocator, item);
        goto next;
      }
      item = cubec_read_ast_function_declarator(allocator, &current, end);
      if (item) {
        if (item->type == CUBEC_NODE_TYPE_ERROR) {
          err = item;
          goto onerror;
        }
        cubec_ast_list_node_append(node->methods, allocator, item);
        goto next;
      }
      item = cubec_read_ast_statement_declaration(allocator, &current, end);
      if (item) {
        if (item->type == CUBEC_NODE_TYPE_ERROR) {
          err = item;
          goto onerror;
        }
        cubec_ast_list_node_append(node->attributes, allocator, item);
        goto next;
      }
      item = cubec_read_ast_struct_field(allocator, &current, end);
      if (item) {
        if (item->type == CUBEC_NODE_TYPE_ERROR) {
          err = item;
          goto onerror;
        }
        cubec_ast_list_node_append(node->fields, allocator, item);
        current.offset++;
        current.column++;
        goto next;
      }
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid struct field");
      goto onerror;
    next:
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
                                 "Invalid struct declarator, missing '}'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  cubec_ast_set_parent(node->identifier, &node->super);
  cubec_ast_set_parent(node->fields, &node->super);
  cubec_ast_set_parent(node->methods, &node->super);
  cubec_ast_set_parent(node->attributes, &node->super);
  cubec_ast_set_parent(node->decorators, &node->super);
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}