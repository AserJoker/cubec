#include "ast/interface_declarator.h"
#include "ast/literal_identifier.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/type.h"
#include "ast/variable_declarator.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
static void
cubec_ast_interface_declarator_dispose(cubec_ast_interface_declarator_t self,
                                       cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->args);
  cubec_allocator_free(allocator, self->type);
  cubec_allocator_free(allocator, self->ptr);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_interface_declarator_t
cubec_create_ast_interface_declarator(cubec_allocator_t allocator) {
  cubec_ast_interface_declarator_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_interface_declarator_t),
      (cubec_dispose_fn_t)cubec_ast_interface_declarator_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_INTERFACE_DECLARATOR;
  cubec_ast_set_field(self, allocator, type);
  cubec_ast_set_field(self, allocator, args);
  cubec_ast_set_field(self, allocator, ptr);
  self->args = cubec_create_ast_list_node(allocator);
  return self;
}

cubec_ast_node_t cubec_read_ast_interface_declarator(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_interface_declarator_t node =
      cubec_create_ast_interface_declarator(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t token =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!token) {
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (!cubec_location_is(token->loc, "func")) {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset == '*') {
    token = cubec_read_ast_literal_symbol(allocator, &current, end);
    if (token) {
      if (token->type == CUBEC_NODE_TYPE_ERROR) {
        err = token;
        goto onerror;
      }
      if (!cubec_location_is(token->loc, "*")) {
        current = token->loc.begin;
        cubec_allocator_free(allocator, token);
      } else {
        node->ptr = token;
        err = cubec_ast_skip_all(allocator, &current, end);
        if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
          return err;
        }
      }
    } else {
      goto onerror;
    }
  }
  if (*current.offset != '(') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ')') {
    for (;;) {
      cubec_ast_node_t arg = NULL;
      if (!arg) {
        arg = cubec_read_ast_literal_symbol(allocator, &current, end);
        if (arg && !cubec_location_is(arg->loc, "...")) {
          current = arg->loc.begin;
          cubec_allocator_free(allocator, arg);
          arg = NULL;
        }
      }
      if (!arg) {
        arg = cubec_read_ast_variable_declarator(allocator, &current, end);
      }
      if (!arg) {
        arg = cubec_read_ast_type(allocator, &current, end);
      }
      if (!arg) {
        goto onerror;
      }
      if (arg->type == CUBEC_NODE_TYPE_ERROR) {
        goto onerror;
      }
      cubec_ast_list_node_append(node->args, allocator, arg);
      err = cubec_ast_skip_all(allocator, &current, end);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
      if (*current.offset == ')') {
        break;
      }
      if (*current.offset != ',') {
        goto onerror;
      }
      current.offset++;
      current.column++;
      err = cubec_ast_skip_all(allocator, &current, end);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
    }
  }
  if (*current.offset != ')') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ':') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  cubec_ast_node_t return_type = cubec_read_ast_type(allocator, &current, end);
  if (!return_type) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid interface expression");
  }
  if (return_type->type == CUBEC_NODE_TYPE_ERROR) {
    err = return_type;
    goto onerror;
  }
  node->type = return_type;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset == '{') {
    goto onerror;
  }
  current = return_type->loc.end;
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  cubec_ast_set_parent(node->type, &node->super);
  cubec_ast_set_parent(node->args, &node->super);
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}