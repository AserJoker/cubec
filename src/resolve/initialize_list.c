#include "resolve/initialize_list.h"
#include "ast/expression_group.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/location.h"
#include "engine/array.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/union.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include "resolve/type.h"
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
static value_t resolve_array_initialize(context_t ctx, ast_node_t type_node,
                                        ast_node_t fields) {
  allocator_t allocator = context_get_allocator(ctx);
  array_t items = create_array(allocator, NULL);
  for (size_t idx = 0; idx < ast_get_length(fields); idx++) {
    ast_node_t field = ast_get_item(fields, idx);
    if (field->type == NODE_TYPE_INITIALIZE_FIELD) {
      return create_comptime_error(ctx, field, "invalid initialize argument");
    } else if (field->type == NODE_TYPE_EXPRESSION_SPREAD) {
      ast_node_t expression = ast_get_child(field, "expression");
      value_t arr = resolve_expression(ctx, expression);
      if (context_is_comptime(ctx) && !value_is_comptime(arr)) {
        allocator_free(allocator, items);
        return create_comptime_error(ctx, expression, "value is not comptime");
      }
      type_t arr_type = value_get_type(arr);
      if (type_get_kind(arr_type) != TYPE_KIND_ARRAY) {
        allocator_free(allocator, items);
        return create_comptime_error(ctx, expression, "value is not array");
      }
      size_t len = array_type_get_length(arr_type);
      for (size_t idx = 0; idx < len; idx++) {
        value_t val =
            value_get(arr, ctx, create_comptime_u64(ctx, idx, false, NULL));
        if (value_is_error(val)) {
          allocator_free(allocator, items);
          return convert_comptime_error(ctx, expression, val);
        }
        array_push(items, val);
      }
    } else {
      value_t value = resolve_expression(ctx, field);
      if (value_is_error(value) || value_is_interrupt(value)) {
        allocator_free(allocator, items);
        return value;
      }
      if (context_is_comptime(ctx) && !value_is_comptime(value)) {
        allocator_free(allocator, items);
        return create_comptime_error(ctx, field, "value is not comptime");
      }
      if (value_is_writer(value)) {
        field->type = NODE_TYPE_VALUE;
        allocator_free(allocator, field->data);
        field->data = value_clone(value, allocator);
      }
      array_push(items, value);
    }
  }
  if (type_node->type == NODE_TYPE_ARRAY_DECLARATOR) {
    size_t len = array_get_size(items);
    ast_node_t length = ast_get_child(type_node, "length");
    if (length->type == NODE_TYPE_LITERAL_IDENTIFIER &&
        location_is(length->loc, "_")) {
      allocator_free(allocator, length->data);
      length->type = NODE_TYPE_VALUE;
      length->value = create_value(allocator, context_load_type(ctx, "u64"),
                                   false, &len, true);
    }
  }
  value_t vtype = resolve_type(ctx, type_node);
  if (value_is_error(vtype) || value_is_interrupt(vtype)) {
    allocator_free(allocator, items);
    return vtype;
  }
  if (type_node->type != NODE_TYPE_VALUE) {
    allocator_free(allocator, type_node->data);
    type_node->value = value_clone(vtype, allocator);
    type_node->type = NODE_TYPE_VALUE;
  }
  type_t type = *(type_t *)value_get_data(vtype);
  type_t item_type = array_type_get_type(type);
  for (size_t idx = 0; idx < array_get_size(items); idx++) {
    value_t item = array_get(items, idx);
    item = value_safe_convert(item, ctx, item_type);
    if (value_is_error(item)) {
      allocator_free(allocator, items);
      return convert_comptime_error(ctx, fields, item);
    }
    array_set(items, idx, item);
  }
  if (context_is_comptime(ctx)) {
    uint8_t data[type_get_size(type)];
    memset(data, 0, type_get_size(type));
    for (size_t idx = 0; idx < array_get_size(items); idx++) {
      value_t item = array_get(items, idx);
      size_t offset = idx * type_get_size(item_type);
      memcpy(data + offset, value_get_data(item), type_get_size(item_type));
    }
    allocator_free(allocator, items);
    return context_create_value(ctx, type, data, false, true, NULL);
  } else {
    allocator_free(allocator, items);
    return context_create_value(ctx, type, NULL, false, true, NULL);
  }
}
static value_t resolve_struct_initialize(context_t ctx, type_t type,
                                         ast_node_t fields) {
  allocator_t allocator = context_get_allocator(ctx);
  if (context_is_comptime(ctx)) {
    uint8_t data[type_get_size(type)];
    for (size_t idx = 0; idx < ast_get_length(fields); idx++) {
      ast_node_t field = ast_get_item(fields, idx);
      if (field->type == NODE_TYPE_INITIALIZE_FIELD) {
        ast_node_t identifier = ast_get_child(field, "identifier");
        ast_node_t initialize = ast_get_child(field, "initialize");
        char *name = location_get(identifier->loc, allocator);
        struct_field_t f = struct_type_get_field(type, name);
        if (!f) {
          value_t err = create_comptime_error(ctx, field,
                                              "no member '%s' in struct", name);
          allocator_free(allocator, name);
          return err;
        }
        allocator_free(allocator, name);
        value_t value = resolve_expression(ctx, initialize);
        if (value_is_error(value) || value_is_interrupt(value)) {
          return value;
        }
        if (!value_is_comptime(value)) {
          return create_comptime_error(ctx, initialize,
                                       "value is not comptime");
        }
        value = value_safe_convert(value, ctx, f->type);
        if (value_is_error(value)) {
          return convert_comptime_error(ctx, initialize, value);
        }
        if (value_is_writer(value)) {
          initialize->type = NODE_TYPE_VALUE;
          allocator_free(allocator, initialize->data);
          initialize->data = value_clone(value, allocator);
        }
        memcpy(data + f->offset, value_get_data(value), type_get_size(f->type));
      } else if (field->type == NODE_TYPE_EXPRESSION_SPREAD) {
        ast_node_t expression = ast_get_child(field, "expression");
        value_t obj = resolve_expression(ctx, expression);
        if (value_is_error(obj) || value_is_interrupt(obj)) {
          return obj;
        }
        if (!value_is_comptime(obj)) {
          return create_comptime_error(ctx, expression,
                                       "value is not comptime");
        }
        type_t obj_type = value_get_type(obj);
        if (type_get_kind(obj_type) != TYPE_KIND_STRUCT) {
          return create_comptime_error(ctx, expression, "value is not struct");
        }
        array_t obj_fields = struct_type_get_fields(obj_type);
        for (size_t idx = 0; idx < array_get_size(obj_fields); idx++) {
          struct_field_t f = array_get(obj_fields, idx);
          value_t value = value_get_field(obj, ctx, f->name);
          if (value_is_error(value) || value_is_interrupt(value)) {
            return convert_comptime_error(ctx, expression, value);
          }
          if (!value_is_comptime(value)) {
            return create_comptime_error(ctx, expression,
                                         "value is not comptime");
          }
          value = value_safe_convert(value, ctx, f->type);
          if (value_is_error(value)) {
            return convert_comptime_error(ctx, expression, value);
          }
          memcpy(data + f->offset, value_get_data(value),
                 type_get_size(f->type));
        }
      } else {
        return create_comptime_error(ctx, field, "invalid initialize argument");
      }
    }
    return context_create_value(ctx, type, data, false, true, NULL);
  } else {
    for (size_t idx = 0; idx < ast_get_length(fields); idx++) {
      ast_node_t field = ast_get_item(fields, idx);
      if (field->type == NODE_TYPE_INITIALIZE_FIELD) {
        ast_node_t identifier = ast_get_child(field, "identifier");
        ast_node_t initialize = ast_get_child(field, "initialize");
        char *name = location_get(identifier->loc, allocator);
        struct_field_t f = struct_type_get_field(type, name);
        if (!f) {
          value_t err = create_comptime_error(ctx, field,
                                              "no member '%s' in struct", name);
          allocator_free(allocator, name);
          return err;
        }
        allocator_free(allocator, name);
        value_t value = resolve_expression(ctx, initialize);
        if (value_is_error(value) || value_is_interrupt(value)) {
          return value;
        }
        value = value_safe_convert(value, ctx, f->type);
        if (value_is_error(value)) {
          return convert_comptime_error(ctx, initialize, value);
        }
        if (value_is_writer(value)) {
          field->type = NODE_TYPE_VALUE;
          allocator_free(allocator, field->data);
          field->data = value_clone(value, allocator);
        }
      } else if (field->type == NODE_TYPE_EXPRESSION_SPREAD) {
        ast_node_t expression = ast_get_child(field, "expression");
        value_t obj = resolve_expression(ctx, expression);
        if (value_is_error(obj) || value_is_interrupt(obj)) {
          return obj;
        }
        type_t obj_type = value_get_type(obj);
        if (type_get_kind(obj_type) != TYPE_KIND_STRUCT) {
          return create_comptime_error(ctx, expression, "value is not struct");
        }
        array_t obj_fields = struct_type_get_fields(obj_type);
        for (size_t idx = 0; idx < array_get_size(obj_fields); idx++) {
          struct_field_t f = array_get(obj_fields, idx);
          value_t value = value_get_field(obj, ctx, f->name);
          value = value_safe_convert(value, ctx, f->type);
          if (value_is_error(value)) {
            return convert_comptime_error(ctx, expression, value);
          }
        }
      } else {
        return create_comptime_error(ctx, field, "invalid initialize argument");
      }
    }
    return context_create_value(ctx, type, NULL, false, false, NULL);
  }
}

static value_t resolve_union_initialize(context_t ctx, type_t type,
                                        ast_node_t fields) {
  allocator_t allocator = context_get_allocator(ctx);
  if (ast_get_length(fields) > 1) {
    return create_comptime_error(ctx, fields, "too many initialize arguments");
  }
  value_t init = NULL;
  if (ast_get_length(fields) == 1) {
    char *name = NULL;
    ast_node_t field = ast_get_item(fields, 0);
    if (field->type != NODE_TYPE_INITIALIZE_FIELD) {
      return create_comptime_error(ctx, field, "invalid initialize argument");
    }
    ast_node_t identifier = ast_get_child(field, "identifier");
    ast_node_t initialize = ast_get_child(field, "initialize");
    init = resolve_expression(ctx, initialize);
    if (value_is_error(init) || value_is_interrupt(init)) {
      return init;
    }
    name = location_get(identifier->loc, allocator);
    union_field_t f = union_type_get_field(type, name);
    if (!f) {
      value_t err = create_comptime_error(ctx, identifier,
                                          "no member '%s' in union", name);
      allocator_free(allocator, name);
      return err;
    }
    init = value_safe_convert(init, ctx, f->type);
    if (value_is_error(init)) {
      allocator_free(allocator, name);
      return convert_comptime_error(ctx, field, init);
    }
    if (value_is_writer(init)) {
      field->type = NODE_TYPE_VALUE;
      allocator_free(allocator, field->data);
      field->data = value_clone(init, allocator);
    }
    allocator_free(allocator, name);
  }
  if (context_is_comptime(ctx)) {
    uint8_t data[type_get_size(type)];
    memset(data, 0, type_get_size(type));
    if (init) {
      type_t value_type = value_get_type(init);
      memcpy(data, value_get_data(init), type_get_size(value_type));
    }
    return context_create_value(ctx, type, data, false, true, NULL);
  } else {
    return context_create_value(ctx, type, NULL, false, true, NULL);
  }
}
static value_t resolve_general_initialize(context_t ctx, type_t type,
                                          ast_node_t fields) {
  allocator_t allocator = context_get_allocator(ctx);
  if (ast_get_length(fields) > 1) {
    return create_comptime_error(ctx, fields, "too many initialize arguments");
  }
  value_t init = NULL;
  if (ast_get_length(fields) == 1) {
    ast_node_t field = ast_get_item(fields, 0);
    init = resolve_expression(ctx, field);
    if (value_is_error(init) || value_is_interrupt(init)) {
      return init;
    }
    init = value_safe_convert(init, ctx, type);
    if (value_is_error(init)) {
      return convert_comptime_error(ctx, field, init);
    }
    if (value_is_writer(init)) {
      field->type = NODE_TYPE_VALUE;
      allocator_free(allocator, field->data);
      field->data = value_clone(init, allocator);
    }
  }
  if (context_is_comptime(ctx)) {
    uint8_t data[type_get_size(type)];
    memset(data, 0, type_get_size(type));
    if (init) {
      memcpy(data, value_get_data(init), type_get_size(type));
    }
    return context_create_value(ctx, type, data, false, true, NULL);
  } else {
    return context_create_value(ctx, type, NULL, false, false, NULL);
  }
}
value_t resolve_initialize_list(context_t ctx, ast_node_t node) {
  allocator_t allocator = context_get_allocator(ctx);
  ast_node_t type_node = ast_get_child(node, "type");
  ast_node_t fields = ast_get_child(node, "fields");
  type_node = ast_unwrap_group(type_node);
  if (type_node->type == NODE_TYPE_ARRAY_DECLARATOR) {
    return resolve_array_initialize(ctx, type_node, fields);
  }
  value_t vtype = resolve_type(ctx, type_node);
  if (value_is_error(vtype) || value_is_interrupt(vtype)) {
    return vtype;
  }
  if (type_node->type != NODE_TYPE_VALUE) {
    allocator_free(allocator, type_node->data);
    type_node->value = value_clone(vtype, allocator);
    type_node->type = NODE_TYPE_VALUE;
  }
  type_t type = *(type_t *)value_get_data(vtype);
  if (type_get_kind(type) == TYPE_KIND_STRUCT) {
    return resolve_struct_initialize(ctx, type, fields);
  } else if (type_get_kind(type) == TYPE_KIND_UNION) {
    return resolve_union_initialize(ctx, type, fields);
  } else if (type_get_kind(type) == TYPE_KIND_ARRAY) {
    return resolve_array_initialize(ctx, type_node, fields);
  } else {
    return resolve_general_initialize(ctx, type, fields);
  }
}