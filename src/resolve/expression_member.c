#include "resolve/expression_member.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/union.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include <stdbool.h>
value_t resolve_expression_member(context_t ctx, ast_node_t node) {
  ast_node_t host = ast_get_child(node, "host");
  ast_node_t field = ast_get_child(node, "field");
  allocator_t allocator = context_get_allocator(ctx);
  value_t obj = resolve_expression(ctx, host);
  if (value_is_error(obj)) {
    return obj;
  }
  type_t type = value_get_type(obj);
  value_t result = NULL;
  char *name = location_get(field->loc, allocator);
  if (value_type_is(obj, VALUE_TYPE_STRUCT)) {
    struct_field_t field = struct_type_get_field(type, name);
    if (!field) {
      char *type_name = type_to_string(type, allocator);
      result = create_compile_error(ctx, node, "no member %s in type %s",
                                    type_name, name);
      allocator_free(allocator, type_name);
    } else {
      result = context_create_value(ctx, field->type, false, NULL, NULL);
    }
  } else if (value_type_is(obj, VALUE_TYPE_UNION)) {
    union_field_t field = union_type_get_field(type, name);
    if (!field) {
      char *type_name = type_to_string(type, allocator);
      result = create_compile_error(ctx, node, "no member %s in type %s",
                                    type_name, name);
      allocator_free(allocator, type_name);
    } else {
      result = context_create_value(ctx, field->type, false, NULL, NULL);
    }
  } else if (value_type_is(obj, VALUE_TYPE_TYPE)) {
    result = value_get_field(obj, ctx, name);
    if (value_is_error(result)) {
      result = convert_compile_error(ctx, node, result);
    }
  } else {
    char *type_name = type_to_string(type, allocator);
    result = create_compile_error(ctx, node, "no member %s in type %s",
                                  type_name, name);
    allocator_free(allocator, type_name);
  }
  allocator_free(allocator, name);
  return result;
}