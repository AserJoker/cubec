#include "resolve/initialize_list.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/arr.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include "resolve/type.h"
#include <stdbool.h>
#include <string.h>
value_t resolve_initialize_list(context_t ctx, ast_node_t node) {
  ast_node_t type_node = ast_get_child(node, "type");
  ast_node_t fields = ast_get_child(node, "fields");
  if (ast_get_length(fields)) {
    type_t type = NULL;
    if (type_node) {
      if (type_node->type == NODE_TYPE_ARRAY_DECLARATOR) {
        ast_node_t length = ast_get_child(type_node, "length");
        if (node_location_is(length, "_")) {
          value_t len =
              create_comptime_u64(ctx, ast_get_length(fields), false, NULL);
          len = value_clone(len, ctx->allocator);
          allocator_free(ctx->allocator, length->data);
          length->value = len;
          length->type = NODE_TYPE_VALUE;
        }
      }
      value_t vtype = resolve_type(ctx, type_node);
      if (vtype->type->kind == TYPE_KIND_ERROR) {
        return vtype;
      }
      type = *(type_t *)vtype->data;
    }
    if (type && type->kind == TYPE_KIND_ARRAY) {
      uint8_t data[type->size];
      type_t base_type = arr_type_get_type(type);
      for (size_t idx = 0; idx < ast_get_length(fields); idx++) {
        ast_node_t field = ast_get_item(fields, idx);
        if (field->type == NODE_TYPE_INITIALIZE_FIELD) {
          return create_comptime_error(ctx, node_get_location(field),
                                       "invalid array item");
        } else {
          value_t item = resolve_expression(ctx, field);
          if (item->type->kind == TYPE_KIND_ERROR) {
            return item;
          }
          item = value_safe_convert(item, ctx, base_type);
          if (item->type->kind == TYPE_KIND_ERROR) {
            return item;
          }
          if (ctx->comptime) {
            if (!item->comptime) {
              return create_comptime_error(ctx, node_get_location(field),
                                           "value is not comptime");
            } else {
              memcpy(data + idx * base_type->size, item->data, base_type->size);
            }
          }
        }
      }
      if (ctx->comptime) {
        return context_create_comptime_value(ctx, type, data, false, NULL);
      } else {
        return context_create_value(ctx, type, false, NULL);
      }
    } else if (!type) {
      typedef struct {
        ast_node_t key;
        value_t value;
      } KV;
      KV items[ast_get_length(fields)];
      for (size_t idx = 0; idx < ast_get_length(fields); idx++) {
        ast_node_t field = ast_get_item(fields, idx);
        if (field->type != NODE_TYPE_INITIALIZE_FIELD) {
          return create_comptime_error(ctx, node_get_location(field),
                                       "invalid initialize field");
        }
        ast_node_t identifier = ast_get_child(field, "identifier");
        ast_node_t value = ast_get_child(field, "value");
        items[idx].key = identifier;
        value_t val = resolve_expression(ctx, value);
        if (val->type->kind == TYPE_KIND_ERROR) {
          return val;
        }
        if (ctx->comptime && !val->comptime) {
          return create_comptime_error(ctx, node_get_location(field),
                                       "value is not comptime");
        }
        items[idx].value = val;
      }
      type = create_struct_type(ctx, NULL);
      for (size_t idx = 0; idx < ast_get_length(fields); idx++) {
        char *name =
            location_get(node_get_location(items[idx].key), ctx->allocator);
        struct_type_add_field(ctx, type, name, items[idx].value->type, true,
                              items[idx].value->mut);
        allocator_free(ctx->allocator, name);
      }
      uint8_t data[type->size];
      for (size_t idx = 0; idx < ast_get_length(fields); idx++) {
        char *name =
            location_get(node_get_location(items[idx].key), ctx->allocator);
        struct_field_t f = struct_type_get_field(type, name);
        allocator_free(ctx->allocator, name);
        if (ctx->comptime) {
          memcpy(data + f->offset, items[idx].value->data, f->type->size);
        }
      }
      if (ctx->comptime) {
        return context_create_comptime_value(ctx, type, data, false, NULL);
      } else {
        return context_create_value(ctx, type, false, NULL);
      }
    } else {
      uint8_t data[type->size];
      for (size_t idx = 0; idx < ast_get_length(fields); idx++) {
        ast_node_t field = ast_get_item(fields, idx);
        if (field->type != NODE_TYPE_INITIALIZE_FIELD) {
          return create_comptime_error(ctx, node_get_location(field),
                                       "invalid initialize field");
        }
        ast_node_t identifier = ast_get_child(field, "identifier");
        ast_node_t value = ast_get_child(field, "value");
        char *name =
            location_get(node_get_location(identifier), ctx->allocator);
        struct_field_t f = struct_type_get_field(type, name);
        allocator_free(ctx->allocator, name);
        if (!f) {
          return create_comptime_error(ctx, node_get_location(field),
                                       "no member '%s' in struct '%s'", name,
                                       type->name);
        }
        value_t val = resolve_expression(ctx, value);
        if (val->type->kind == TYPE_KIND_ERROR) {
          return val;
        }
        val = value_safe_convert(val, ctx, f->type);
        if (val->type->kind == TYPE_KIND_ERROR) {
          return val;
        }
        if (ctx->comptime) {
          if (!val->comptime) {
            return create_comptime_error(ctx, node_get_location(field),
                                         "value is not comptime");
          } else {
            memcpy(data + f->offset, val->data, f->type->size);
          }
        }
      }
      if (ctx->comptime) {
        return context_create_comptime_value(ctx, type, data, false, NULL);
      } else {
        return context_create_value(ctx, type, false, NULL);
      }
    }
  } else {
    type_t type = NULL;
    if (type_node) {
      if (type_node->type == NODE_TYPE_ARRAY_DECLARATOR) {
        ast_node_t length = ast_get_child(type_node, "length");
        if (node_location_is(length, "_")) {
          value_t len = create_comptime_u64(ctx, 0, false, NULL);
          len = value_clone(len, ctx->allocator);
          allocator_free(ctx->allocator, length->data);
          length->value = len;
          length->type = NODE_TYPE_VALUE;
        }
      }
      value_t vtype = resolve_type(ctx, type_node);
      if (vtype->type->kind == TYPE_KIND_ERROR) {
        return vtype;
      }
      type = *(type_t *)vtype->data;
    } else {
      type = create_struct_type(ctx, NULL);
    }
    return context_create_comptime_value(ctx, type, NULL, false, NULL);
  }
}