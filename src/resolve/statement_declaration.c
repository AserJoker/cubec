#include "resolve/statement_declaration.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/array.h"
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
  bool mut = !location_is(declar_type->loc, "const");
  bool comptime = false;
  if (kind) {
    comptime = location_is(kind->loc, "comptime");
  }
  comptime |= context_is_comptime(ctx);
  comptime = context_set_comptime(ctx, comptime);
  for (size_t idx = 0; idx < ast_get_length(declarations); idx++) {
    ast_node_t declarator = ast_get_item(declarations, idx);
    ast_node_t identifier = ast_get_child(declarator, "identifier");
    ast_node_t type = ast_get_child(declarator, "type");
    ast_node_t initialize = ast_get_child(declarator, "initialize");
    if (location_is(identifier->loc, "_")) {
      value_t err = create_comptime_error(
          ctx, identifier, "'_' only to ignore values in expressions");
      if (comptime) {
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
        if (comptime) {
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
    value_t value = resolve_expression(ctx, initialize);
    if (value_is_error(value)) {
      if (comptime) {
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
      return create_comptime_error(ctx, initialize, "value is not comptime");
    }
    type_t value_type = value_get_type(value);
    if (type_get_kind(value_type) == TYPE_KIND_VOID) {
      value_t err = create_comptime_error(ctx, initialize, "value is void");
      if (comptime) {
        return err;
      } else {
        context_push_error(ctx, err);
        continue;
      }
    }
    if (type_get_kind(value_type) == TYPE_KIND_TYPE &&
        !context_is_comptime(ctx)) {
      value_t err = create_comptime_error(
          ctx, initialize, "type value only declared with comptime");
      if (comptime) {
        return err;
      } else {
        context_push_error(ctx, err);
        continue;
      }
    }
    if (type_get_kind(value_type) == TYPE_KIND_COMPTIME_FUNCTION &&
        !context_is_comptime(ctx)) {
      value_t err =
          create_comptime_error(ctx, initialize, "value is comptime function");
      if (comptime) {
        return err;
      } else {
        context_push_error(ctx, err);
        continue;
      }
    }
    if (!context_is_comptime(ctx) &&
        type_get_kind(value_type) == TYPE_KIND_STR) {
      const char *str = *(const char **)value_get_data(value);
      size_t len = strlen(str) + 1;
      char buf[len];
      strcpy(buf, str);
      buf[len - 1] = 0;
      type_t arr_type =
          create_array_type(ctx, context_load_type(ctx, "u8"), len);
      value = context_create_value(ctx, arr_type, buf, false, true, NULL);
      value_type = arr_type;
    }
    if (type) {
      value_t vdst_type = resolve_type(ctx, type);
      if (value_is_error(vdst_type)) {
        if (comptime) {
          return vdst_type;
        } else {
          context_push_error(ctx, vdst_type);
          continue;
        }
      }
      if (value_is_interrupt(vdst_type)) {
        return vdst_type;
      }
      type_t dst_type = *(type_t *)value_get_data(vdst_type);
      if (type_get_kind(dst_type) == TYPE_KIND_STR &&
          !context_is_comptime(ctx)) {
        value_t err = create_comptime_error(
            ctx, initialize, "str value only declared with comptime");
        if (comptime) {
          return err;
        } else {
          context_push_error(ctx, err);
          continue;
        }
      }
      value = value_safe_convert(value, ctx, dst_type);
      if (value_is_error(value)) {
        value_t err = convert_comptime_error(ctx, initialize, value);
        if (comptime) {
          return err;
        } else {
          context_push_error(ctx, err);
          continue;
        }
      }
      value_type = dst_type;
    }
    char *name = location_get(identifier->loc, allocator);
    if (context_get_type(ctx) == CONTEXT_TYPE_FUNCTION) {
      if (context_is_comptime(ctx)) {
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
        if (!context_is_comptime(ctx)) {
          value = context_create_value(ctx, value_type, NULL, mut, false, name);
        }
        struct_type_add_attribute(self, allocator, name, value,
                                  pub_node != NULL);
      }
    }
    allocator_free(allocator, name);
    if (value_is_error(value)) {
      value = convert_comptime_error(ctx, identifier, value);
      if (comptime) {
        return value;
      } else {
        context_push_error(ctx, value);
        continue;
      }
    }
  }
  context_set_comptime(ctx, comptime);
  if (result) {
    return create_comptime_error(ctx, node, "declartion statement error");
  }
  return context_get_undefined(ctx);
}