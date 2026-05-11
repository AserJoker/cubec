#include "resolve/statement_declaration.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include "resolve/type.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
value_t resolve_statement_declaration(context_t ctx, ast_node_t node) {
  value_t result = NULL;
  allocator_t allocator = context_get_allocator(ctx);
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t declarations = ast_get_child(node, "declarations");
  ast_node_t declar_type = ast_get_child(node, "type");
  ast_node_t pub_node = ast_get_child(node, "pub");
  if (pub_node && context_get_type(ctx) != CONTEXT_TYPE_STRUCT) {
    return create_comptime_error(ctx, pub_node, "invalid pub declaration");
  }
  bool comptime = kind && location_is(kind->loc, "comptime");
  bool mut = !location_is(declar_type->loc, "const");
  for (size_t idx = 0; idx < ast_get_length(declarations); idx++) {
    ast_node_t declarator = ast_get_item(declarations, idx);
    ast_node_t identifier = ast_get_child(declarator, "identifier");
    ast_node_t type = ast_get_child(declarator, "type");
    ast_node_t initialize = ast_get_child(declarator, "initialize");
    if (location_is(identifier->loc, "_")) {
      value_t err = create_comptime_error(
          ctx, identifier, "'_' only to ignore values in expressions");
      if (context_is_comptime(ctx)) {
        return err;
      } else {
        context_push_error(ctx, err);
        continue;
      }
    }
    if (initialize->type == NODE_TYPE_INITIALIZE_LIST) {
      ast_node_t itype = ast_get_child(initialize, "type");
      if (!itype && !type) {
        value_t err = create_comptime_error(ctx, declarator,
                                            "missing type for initialize list");
        if (context_is_comptime(ctx)) {
          return err;
        } else {
          context_push_error(ctx, err);
          continue;
        }
      }
      if (!itype) {
        type = ast_move_child(declarator, "type");
        ast_add_child(allocator, initialize, "type", type);
        type = NULL;
      }
    }
    value_t value = NULL;
    if (kind && location_is(kind->loc, "comptime")) {
      bool is_comptime = context_set_comptime(ctx, true);
      value = resolve_expression(ctx, initialize);
      context_set_comptime(ctx, is_comptime);
    } else {
      value = resolve_expression(ctx, initialize);
    }
    if (value_is_error(value)) {
      if (context_is_comptime(ctx)) {
        return value;
      } else {
        context_push_error(ctx, value);
        continue;
      }
    }
    if (value_is_error(value)) {
      return value;
    }
    if (context_is_comptime(ctx) && !value_is_comptime(value)) {
      value_t err =
          create_comptime_error(ctx, initialize, "value is not comptime");
      if (context_is_comptime(ctx)) {
        return err;
      } else {
        context_push_error(ctx, err);
        continue;
      }
    }
    type_t value_type = value_get_type(value);
    if (type_get_kind(value_type) == TYPE_KIND_VOID) {
      value_t err = create_comptime_error(ctx, initialize, "value is void");
      if (context_is_comptime(ctx)) {
        return err;
      } else {
        context_push_error(ctx, err);
        continue;
      }
    }
    if (type_get_kind(value_type) == TYPE_KIND_TYPE &&
        !context_is_comptime(ctx) && !comptime) {
      value_t err = create_comptime_error(
          ctx, initialize, "type value only declared with comptime");
      if (context_is_comptime(ctx)) {
        return err;
      } else {
        context_push_error(ctx, err);
        continue;
      }
    }
    if ((type_get_kind(value_type) == TYPE_KIND_COMPTIME_FUNCTION ||
         type_get_kind(value_type) == TYPE_KIND_TEMPLATE_FUNCTION ||
         type_get_kind(value_type) == TYPE_KIND_PTR ||
         type_get_kind(value_type) == TYPE_KIND_PARRAY ||
         type_get_kind(value_type) == TYPE_KIND_OPAQUE ||
         type_get_kind(value_type) == TYPE_KIND_SLICE) &&
        !context_is_comptime(ctx) && !comptime) {
      value_t err = create_comptime_error(ctx, initialize, "value is comptime");
      if (context_is_comptime(ctx)) {
        return err;
      } else {
        context_push_error(ctx, err);
        continue;
      }
    }
    if (type) {
      value_t vdst_type = resolve_type(ctx, type);
      if (value_is_error(vdst_type)) {
        if (context_is_comptime(ctx)) {
          return vdst_type;
        } else {
          context_push_error(ctx, vdst_type);
          continue;
        }
      }
      type_t dst_type = *(type_t *)value_get_data(vdst_type);
      if (type_get_kind(dst_type) == TYPE_KIND_STR &&
          !context_is_comptime(ctx)) {
        value_t err = create_comptime_error(
            ctx, initialize, "str value only declared with comptime");
        if (context_is_comptime(ctx)) {
          return err;
        } else {
          context_push_error(ctx, err);
          continue;
        }
      }
      value = value_safe_convert(value, ctx, dst_type);
      if (value_is_error(value)) {
        value_t err = convert_comptime_error(ctx, initialize, value);
        if (context_is_comptime(ctx)) {
          return err;
        } else {
          context_push_error(ctx, err);
          continue;
        }
      }
      value_type = dst_type;
    }
    if (type_get_kind(value_type) == TYPE_KIND_PTR ||
        type_get_kind(value_type) == TYPE_KIND_PARRAY ||
        type_get_kind(value_type) == TYPE_KIND_OPAQUE ||
        type_get_kind(value_type) == TYPE_KIND_SLICE) {
      if (mut && !value_is_mut(value)) {
        value_t err = create_comptime_error(
            ctx, initialize, "cannot initialize '%s' with 'const %s'",
            type_get_name(value_type), type_get_name(value_type));
        if (context_is_comptime(ctx)) {
          return err;
        } else {
          context_push_error(ctx, err);
          continue;
        }
      }
    }
    value_t vtype = create_type_value(ctx, value_type, false, NULL);
    ast_node_t _type = create_ast_value_node(allocator, vtype);
    ast_add_child(allocator, declarator, "_type", _type);
    char *name = location_get(identifier->loc, allocator);
    if (context_get_type(ctx) != CONTEXT_TYPE_STRUCT) {
      if (kind && location_is(kind->loc, "comptime")) {
        void *data = value_get_data(value);
        value = context_create_value(ctx, value_type, data, mut, true, name);
      } else {
        value = context_create_value(ctx, value_type, NULL, mut, false, name);
      }
    } else {
      type_t self = context_get_self(ctx);
      if (struct_type_get_attribute(self, name)) {
        value = create_error(ctx, "redefinition of '%s'", name);
      } else {
        if (!kind || !location_is(kind->loc, "comptime")) {
          value = context_create_value(ctx, value_type, NULL, mut, false, name);
        }
        struct_type_add_attribute(self, allocator, name, value,
                                  pub_node != NULL);
      }
    }
    allocator_free(allocator, name);
    if (value_is_error(value)) {
      value = convert_comptime_error(ctx, identifier, value);
      if (context_is_comptime(ctx)) {
        return value;
      } else {
        context_push_error(ctx, value);
        continue;
      }
    }
  }
  if (result) {
    return create_comptime_error(ctx, node, "declartion statement error");
  }
  if (kind && location_is(kind->loc, "comptime")) {
    node->visible = false;
  }
  return context_get_undefined(ctx);
}