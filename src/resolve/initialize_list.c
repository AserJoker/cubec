#include "resolve/initialize_list.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/array.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include "resolve/type.h"
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
value_t resolve_initialize_list(context_t ctx, ast_node_t node) {
  allocator_t allocator = context_get_allocator(ctx);
  ast_node_t type_node = ast_get_child(node, "type");
  ast_node_t fields = ast_get_child(node, "fields");
  if (type_node->type == NODE_TYPE_ARRAY_DECLARATOR) {
    ast_node_t length = ast_get_child(type_node, "length");
    if (location_is(length->loc, "_")) {
      size_t len = ast_get_length(fields);
      if (len == 0) {
        return create_comptime_error(ctx, length, "invalid array size");
      }
      allocator_free(allocator, length->data);
      length->type = NODE_TYPE_VALUE;
      length->value = create_value(allocator, context_load_type(ctx, "u64"),
                                   false, &len, true);
    }
  }
  value_t vtype = resolve_type(ctx, type_node);
  if (value_is_error(vtype) || value_is_interrupt(vtype)) {
    return vtype;
  }
  type_t type = *(type_t *)value_get_data(vtype);
  if (context_is_comptime(ctx)) {
    char data[type_get_size(type)];
    memset(data, 0, type_get_size(type));
    if (ast_get_length(fields) == 0) {
      return context_create_value(ctx, type, data, false, true, NULL);
    }
    if (type_get_kind(type) == TYPE_KIND_STRUCT) {
      return create_comptime_error(ctx, node, "not implement");
    } else if (type_get_kind(type) == TYPE_KIND_UNION) {
      return create_comptime_error(ctx, node, "not implement");
    } else if (type_get_kind(type) == TYPE_KIND_ARRAY) {
      size_t len = array_type_get_length(type);
      if (ast_get_length(fields) > len) {
        return create_comptime_error(ctx, node, "too many initialize fields");
      }
      type_t item_type = array_type_get_type(type);
      for (size_t idx = 0; idx < ast_get_length(fields); idx++) {
        ast_node_t field = ast_get_item(fields, idx);
        ast_node_t identifier = ast_get_child(field, "identifier");
        if (identifier) {
          return create_comptime_error(ctx, field, "%s is not struct or union",
                                       type_get_name(type));
        }
        ast_node_t initialize = ast_get_child(field, "initialize");
        value_t val = resolve_expression(ctx, initialize);
        if (value_is_error(val) || value_is_interrupt(val)) {
          return val;
        }
        if (context_is_comptime(ctx) && !value_is_comptime(val)) {
          return create_comptime_error(ctx, field, "item is not comptime");
        }
        val = value_safe_convert(val, ctx, item_type);
        if (value_is_error(val)) {
          return convert_comptime_error(ctx, field, val);
        }
        memcpy(data + idx * type_get_size(item_type), value_get_data(val),
               type_get_size(item_type));
      }
      return context_create_value(ctx, type, data, false, true, NULL);
    } else {
      if (ast_get_length(fields) > 1) {
        return create_comptime_error(ctx, node, "too many initialize fields");
      }
      ast_node_t field = ast_get_item(fields, 0);
      ast_node_t identifier = ast_get_child(field, "identifier");
      if (identifier) {
        return create_comptime_error(ctx, field, "%s is not struct or union",
                                     type_get_name(type));
      }
      ast_node_t initialize = ast_get_child(field, "initialize");
      value_t val = resolve_expression(ctx, initialize);
      if (value_is_error(val) || value_is_interrupt(val)) {
        return val;
      }
      if (!value_is_comptime(val)) {
        return create_comptime_error(ctx, field, "value is not comptime");
      }
      val = value_safe_convert(val, ctx, type);
      if (value_is_error(val)) {
        return convert_comptime_error(ctx, field, val);
      }
      return val;
    }
  } else {
    if (!ast_get_length(fields)) {
      return context_create_value(ctx, type, NULL, false, true, NULL);
    }
    if (type_get_kind(type) == TYPE_KIND_STRUCT) {
      return create_comptime_error(ctx, node, "not implement");
    } else if (type_get_kind(type) == TYPE_KIND_UNION) {
      return create_comptime_error(ctx, node, "not implement");
    } else if (type_get_kind(type) == TYPE_KIND_ARRAY) {
      size_t len = array_type_get_length(type);
      if (ast_get_length(fields) > len) {
        return create_comptime_error(ctx, node, "too many initialize fields");
      }
      type_t item_type = array_type_get_type(type);
      for (size_t idx = 0; idx < ast_get_length(fields); idx++) {
        ast_node_t field = ast_get_item(fields, idx);
        ast_node_t identifier = ast_get_child(field, "identifier");
        if (identifier) {
          return create_comptime_error(ctx, field, "%s is not struct or union",
                                       type_get_name(type));
        }
        ast_node_t initialize = ast_get_child(field, "initialize");
        value_t val = resolve_expression(ctx, initialize);
        if (value_is_error(val) || value_is_interrupt(val)) {
          return val;
        }
        val = value_safe_convert(val, ctx, item_type);
        if (value_is_error(val)) {
          return convert_comptime_error(ctx, initialize, val);
        }
        if (value_is_writer(val)) {
          allocator_free(allocator, initialize->data);
          initialize->type = NODE_TYPE_VALUE;
          initialize->data = value_clone(val, allocator);
        }
      }
      return context_create_value(ctx, type, NULL, false, true, NULL);
    } else {
      if (ast_get_length(fields) > 1) {
        return create_comptime_error(ctx, node, "too many initialize fields");
      }
      ast_node_t field = ast_get_item(fields, 0);
      ast_node_t identifier = ast_get_child(field, "identifier");
      if (identifier) {
        return create_comptime_error(ctx, field, "%s is not struct or union",
                                     type_get_name(type));
      }
      ast_node_t initialize = ast_get_child(field, "initialize");
      value_t val = resolve_expression(ctx, initialize);
      if (value_is_error(val) || value_is_interrupt(val)) {
        return val;
      }
      if (!value_is_comptime(val)) {
        return create_comptime_error(ctx, initialize, "value is not comptime");
      }
      val = value_safe_convert(val, ctx, type);
      if (value_is_error(val)) {
        return convert_comptime_error(ctx, initialize, val);
      }
      if (value_is_writer(val)) {
        allocator_free(allocator, initialize->data);
        initialize->type = NODE_TYPE_VALUE;
        initialize->value = value_clone(val, allocator);
      }
      return val;
    }
  }
}