#include "resolve/statement_declaration.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
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
      bool comptime = ctx->comptime;
      ctx->comptime = true;
      type = resolve_type(ctx, type_node);
      ctx->comptime = comptime;
      CHECK_ERROR(ctx, type);
    }
    bool is_comptime = ctx->comptime;
    if (kind && node_location_is(kind, "comptime")) {
      ctx->comptime = true;
    }
    value_t value = resolve_expression(ctx, initialize);
    ctx->comptime = is_comptime;
    if (value->type->kind == TYPE_KIND_VOID) {
      value = create_comptime_error(ctx, node_get_location(initialize),
                                    "cannot declar void value");
    }
    CHECK_ERROR(ctx, value);
    if (type) {
      type_t t = *(type_t *)type->data;
      value = value_safe_convert(value, ctx, t);
      if (value->type->kind == TYPE_KIND_ERROR) {
        value =
            convert_comptime_error(ctx, node_get_location(initialize), value);
      }
      CHECK_ERROR(ctx, value);
    }
    type_t t = value->type;
    type = create_type_value(ctx, t, false, NULL);
    type_node = create_ast_value(ctx->allocator, type);
    ast_remove_child(declar, "type");
    ast_add_child(ctx->allocator, declar, "type", type_node);
    value_t variable = NULL;
    bool is_mut = node_location_is(mut, "let");
    char *id = location_get(node_get_location(identifier), ctx->allocator);
    if (ctx->type == CONTEXT_TYPE_FUNCTION) {
      if (ctx->comptime || kind && node_location_is(kind, "comptime")) {
        value = create_comptime_value(ctx->allocator, t, value->data, is_mut);
      } else {
        value = create_value(ctx->allocator, t, is_mut);
      }
      if (pub) {
        allocator_free(ctx->allocator, value);
        value_t err = create_comptime_error(
            ctx, node_get_location(pub), "pub only used in struct or global");
        CHECK_ERROR(ctx, err);
      }
      value_t err = context_declar(ctx, id, value);
      if (err->type->kind == TYPE_KIND_ERROR) {
        err = convert_comptime_error(ctx, node_get_location(node), err);
      }
      CHECK_ERROR(ctx, err);
    } else {
      value_t err = struct_type_add_attribute(
          ctx, ctx->self, id, pub != NULL, initialize, t, is_mut,
          kind && node_location_is(kind, "comptime"));
      if (err->type->kind == TYPE_KIND_ERROR) {
        allocator_free(ctx->allocator, value);
        err = convert_comptime_error(ctx, node_get_location(node), err);
      }
      CHECK_ERROR(ctx, err);
    }
    allocator_free(ctx->allocator, id);
  }
  return create_comptime_void(ctx);
}