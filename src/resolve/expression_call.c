#include "resolve/expression_call.h"
#include "ast/expression_group.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/location.h"
#include "engine/error.h"
#include "engine/function.h"
#include "engine/ptr.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include <stdbool.h>

value_t resolve_expression_call(context_t ctx, ast_node_t node) {
  ast_node_t callee = ast_get_child(node, "callee");
  ast_node_t arguments = ast_get_child(node, "arguments");
  array_t args = create_array(ctx->allocator, NULL);
  callee = ast_unwrap_group(callee);
  value_t val = NULL;
  if (callee->type == NODE_TYPE_EXPRESSION_MEMBER) {
    ast_node_t host = ast_get_child(callee, "host");
    value_t obj = resolve_expression(ctx, host);
    if (obj->type->kind == TYPE_KIND_ERROR) {
      allocator_free(ctx->allocator, args);
      return obj;
    }
    type_t type = obj->type;
    if (obj->type->kind == TYPE_KIND_STRUCT) {
      ast_node_t field = ast_get_child(callee, "field");
      obj = value_addr(obj, ctx);
      array_push(args, obj);
      char *f = location_get(node_get_location(field), ctx->allocator);
      struct_attribute_t attr = struct_type_get_method(type, f);
      if (!attr) {
        val = create_comptime_error(ctx, node_get_location(callee),
                                    "no method '%s' in '%s'", f, type->id);
      } else if (attr->initialize->type == NODE_TYPE_VALUE) {
        val = attr->initialize->value;
      } else if (attr->initialize->type == NODE_TYPE_FUNCTION_DECLARATOR) {
        ast_node_t bind = ast_get_child(attr->initialize, "bind");
        val = bind->value;
      } else {
        val = create_comptime_error(ctx, node_get_location(callee),
                                    "%s.%s is not callable", type->id, f);
      }
      allocator_free(ctx->allocator, f);
      if (val->type->kind == TYPE_KIND_ERROR) {
        allocator_free(ctx->allocator, args);
        return convert_comptime_error(ctx, node_get_location(callee), val);
      }
      callee->vtype = val->type;
    } else if (obj->type->kind == TYPE_KIND_PTR &&
               ptr_type_get_type(type)->kind == TYPE_KIND_STRUCT) {
      ast_node_t field = ast_get_child(callee, "field");
      array_push(args, obj);
      type = ptr_type_get_type(type);
      char *f = location_get(node_get_location(field), ctx->allocator);
      struct_attribute_t attr = struct_type_get_method(type, f);
      if (!attr) {
        val = create_comptime_error(ctx, node_get_location(callee),
                                    "no method '%s' in '%s'", f, type->id);
      } else if (attr->initialize->type == NODE_TYPE_VALUE) {
        val = attr->initialize->value;
      } else if (attr->initialize->type == NODE_TYPE_FUNCTION_DECLARATOR) {
        ast_node_t bind = ast_get_child(attr->initialize, "bind");
        val = bind->value;
      } else {
        val = create_comptime_error(ctx, node_get_location(callee),
                                    "%s.%s is not callable", type->id, f);
      }
      allocator_free(ctx->allocator, f);
      if (val->type->kind == TYPE_KIND_ERROR) {
        allocator_free(ctx->allocator, args);
        return convert_comptime_error(ctx, node_get_location(callee), val);
      }
      callee->vtype = val->type;
    } else {
      val = resolve_expression(ctx, callee);
    }
  } else {
    val = resolve_expression(ctx, callee);
  }
  if (val->type->kind == TYPE_KIND_ERROR) {
    allocator_free(ctx->allocator, args);
    return val;
  }
  bool is_comptime = ctx->comptime;
  if (val->data) {
    function_declar_t declar = *(function_declar_t *)val->data;
    if (declar && declar->kind == FUNCTION_KIND_COMPTIME) {
      ctx->comptime = true;
    }
  }
  for (size_t idx = 0; idx < ast_get_length(arguments); idx++) {
    ast_node_t arg = ast_get_item(arguments, idx);
    value_t value = resolve_expression(ctx, arg);
    if (value->type->kind == TYPE_KIND_ERROR) {
      allocator_free(ctx->allocator, args);
      ctx->comptime = is_comptime;
      return value;
    }
    array_push(args, value);
  }
  value_t result = NULL;
  if (val->type->kind == TYPE_KIND_TEMPLATE) {
    val = template_create_instance(val, ctx, array_get_size(args),
                                   array_get_data(args));
  }
  if (val->type->kind == TYPE_KIND_ERROR) {
    result = val;
  } else {
    result = value_call(val, ctx, array_get_size(args), array_get_data(args));
  }
  allocator_free(ctx->allocator, args);
  if (result->type->kind == TYPE_KIND_ERROR) {
    result = convert_comptime_error(ctx, node_get_location(node), result);
  }
  ctx->comptime = is_comptime;
  if (callee->type != NODE_TYPE_VALUE) {
    ast_node_t bind = create_ast_value(ctx->allocator, val);
    ast_add_child(ctx->allocator, callee, "bind", bind);
  } else if (callee->value->type->kind == TYPE_KIND_TEMPLATE) {
    allocator_free(ctx->allocator, callee->value);
    callee->value = value_clone(val, ctx->allocator);
  }
  return result;
}