#include "ast/enum_field.h"
#include "ast/decorator.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
static void cubec_ast_enum_field_dispose(cubec_ast_enum_field_t self,
                                         cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->decorators);
  cubec_allocator_free(allocator, self->identifier);
  cubec_allocator_free(allocator, self->value);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_enum_field_t
cubec_create_ast_enum_field(cubec_allocator_t allocator) {
  cubec_ast_enum_field_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ast_enum_field_t),
                            (cubec_dispose_fn_t)cubec_ast_enum_field_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_ENUM_FIELD;
  cubec_ast_set_field(self, allocator, identifier);
  cubec_ast_set_field(self, allocator, value);
  cubec_ast_set_field(self, allocator, decorators);
  self->decorators = cubec_create_ast_list_node(allocator);
  return self;
}
cubec_ast_node_t cubec_read_ast_enum_field(cubec_allocator_t allocator,
                                           cubec_position_t *position,
                                           const char *end) {
  cubec_ast_enum_field_t node = cubec_create_ast_enum_field(allocator);
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
  cubec_ast_node_t identifier =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!identifier) {
    goto onerror;
  }
  if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  node->identifier = identifier;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset == '=') {
    current.offset++;
    current.column++;
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
    cubec_ast_node_t value =
        cubec_read_ast_expression2(allocator, &current, end);
    if (!value) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid enum field, missing value");
      goto onerror;
    }
    if (value->type == CUBEC_NODE_TYPE_ERROR) {
      err = value;
      goto onerror;
    }
    node->value = value;
  } else {
    current = identifier->loc.end;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  cubec_ast_set_parent(&node->super, node->identifier);
  cubec_ast_set_parent(&node->super, node->value);
  cubec_ast_set_parent(&node->super, node->decorators);
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}