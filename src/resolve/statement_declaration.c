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
#include "engine/void.h"
#include "resolve/expression.h"
#include "resolve/type.h"
#define CHECK_ERROR(ctx, err)                                                  \
  if (err->type->kind == TYPE_KIND_ERROR) {                                    \
    if (ctx->comptime) {                                                       \
      return err;                                                              \
    } else {                                                                   \
      context_push_error(ctx, err);                                            \
      continue;                                                                \
    }                                                                          \
  }
value_t resolve_statement_declaration(context_t ctx, ast_node_t node) {
  ast_node_t pub = ast_get_child(node, "pub");
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t mut = ast_get_child(node, "mut");
  ast_node_t declarations = ast_get_child(node, "declarations");
  for (size_t idx = 0; idx < ast_get_length(declarations); idx++) {
    ast_node_t declar = ast_get_item(declarations, idx);
    ast_node_t identifier = ast_get_child(declar, "identifier");
    ast_node_t initialize = ast_get_child(declar, "initialize");
    ast_node_t type_node = ast_get_child(declar, "type");
    value_t type = NULL;
    if (type_node) {
      type = resolve_type(ctx, type_node);
      CHECK_ERROR(ctx, type);
    }
    if (initialize->type == NODE_TYPE_INITIALIZE_LIST) {
      ast_node_t type_node = ast_get_child(initialize, "type");
      if (!type_node && !type) {
        value_t err = create_comptime_error(ctx, node_get_location(initialize),
                                            "missing initialize type");
        CHECK_ERROR(ctx, err);
      }
      if (!type_node) {
        type_node =
            create_ast_value(ctx->allocator, value_clone(type, ctx->allocator));
        ast_add_child(ctx->allocator, initialize, "type", type_node);
      }
    }
    bool is_comptime = ctx->comptime;
    if (kind && node_location_is(kind, "comptime")) {
      is_comptime = ctx->comptime;
      ctx->comptime = true;
    }
    value_t value = resolve_expression(ctx, initialize);
    ctx->comptime = is_comptime;
    if (ctx->comptime || ctx->type != CONTEXT_TYPE_FUNCTION) {
      if (!value->comptime) {
        value_t err = create_comptime_error(ctx, node_get_location(initialize),
                                            "value is not comptime");
        CHECK_ERROR(ctx, err);
      }
    }
    CHECK_ERROR(ctx, value);
    if (type) {
      type_t t = *(type_t *)type->data;
      value = value_safe_convert(value, ctx, t);
      CHECK_ERROR(ctx, value);
    }
    type = create_type_value(ctx, value->type, false, NULL);
    type_node = create_ast_value(ctx->allocator, type);
    ast_remove_child(declar, "type");
    ast_add_child(ctx->allocator, declar, "type", type_node);
    if (ctx->comptime || kind && node_location_is(kind, "comptime")) {
      value = create_comptime_value(ctx->allocator, value->type, value->data,
                                    node_location_is(mut, "let"));
    } else {
      value = create_value(ctx->allocator, value->type,
                           node_location_is(mut, "let"));
    }
    if (ctx->type == CONTEXT_TYPE_FUNCTION) {
      if (pub) {
        value_t err = create_comptime_error(
            ctx, node_get_location(pub), "pub only used in struct or global");
        CHECK_ERROR(ctx, err);
      }
      char *id = location_get(node_get_location(identifier), ctx->allocator);
      value_t err = context_declar(ctx, id, value);
      allocator_free(ctx->allocator, id);
      CHECK_ERROR(ctx, err);
    } else {
      char *id = location_get(node_get_location(identifier), ctx->allocator);
      value_t err =
          struct_type_add_attribute(ctx, ctx->self, id, value, pub != NULL);
      allocator_free(ctx->allocator, id);
      CHECK_ERROR(ctx, err);
    }
  }
  return create_comptime_void(ctx);
}