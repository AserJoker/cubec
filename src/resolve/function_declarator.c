#include "resolve/function_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/hash_map.h"
#include "core/location.h"
#include "core/position.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/function.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/type.h"
#include <stdbool.h>

value_t resolve_function_declarator(context_t ctx, ast_node_t node) {
  ast_node_t closure = ast_get_child(node, "closure");
  ast_node_t kind = ast_get_child(node, "kind");
  if (kind && node_location_is(kind, "extern")) {
    ast_node_t arguments = ast_get_child(node, "arguments");
    ast_node_t type = ast_get_child(node, "type");
    ast_node_t mut = ast_get_child(node, "mut");
    ast_node_t identifier = ast_get_child(node, "identifier");
    value_t err = NULL;
    ctype_t return_type = NULL;
    array_t args = create_array(ctx->allocator, &(array_initialize_t){
                                                    .autofree = true,
                                                });
    scope_t current_scope = ctx->current;
    scope_t scope = create_scope(ctx->allocator, ctx->root);
    ctx->current = scope;
    bool variadic = false;
    for (size_t idx = 0; idx < ast_get_length(arguments); idx++) {
      ast_node_t arg = ast_get_item(arguments, idx);
      ast_node_t identifier = ast_get_child(arg, "identifier");
      ast_node_t type = ast_get_child(arg, "type");
      ast_node_t mut = ast_get_child(arg, "mut");
      type_t t = NULL;
      if (type) {
        if (node_location_is(type, "infer")) {
          err = NULL;
          goto onerror;
        }
        value_t vt = resolve_type(ctx, type);
        if (vt->type->kind == TYPE_KIND_ERROR) {
          err = vt;
          goto onerror;
        }
        t = *(type_t *)vt->data;
        if (t->kind == TYPE_KIND_TYPE) {
          err = NULL;
          goto onerror;
        }
        char *name =
            location_get(node_get_location(identifier), ctx->allocator);
        err = context_create_value(ctx, t, mut == NULL, name);
        allocator_free(ctx->allocator, name);
        if (err->type->kind == TYPE_KIND_ERROR) {
          goto onerror;
        }
      }
      ctype_t ctype = create_ctype(ctx->allocator, t, mut == NULL);
      array_push(args, ctype);
      if (arg->type == NODE_TYPE_ARGUMENT_REST) {
        variadic = true;
      }
    }
    value_t vt = resolve_type(ctx, type);
    if (vt->type->kind == TYPE_KIND_ERROR) {
      err = vt;
      goto onerror;
    }
    type_t t = *(type_t *)vt->data;
    return_type = create_ctype(ctx->allocator, t, mut == NULL);
    type_t function_type =
        create_function_type(ctx, return_type, args, variadic, NULL);
    allocator_free(ctx->allocator, args);
    allocator_free(ctx->allocator, return_type);
    ctx->current = current_scope;
    char *id = location_get(node_get_location(identifier), ctx->allocator);
    if (hash_map_get(ctx->functions, id, NULL, NULL)) {
      value_t curr = hash_map_get(ctx->functions, id, NULL, NULL);
      type_t type = curr->type;
      if (!type_is_equal(type, function_type)) {
        allocator_free(ctx->allocator, id);
        return create_comptime_error(ctx, node_get_location(node),
                                     "duplicate extern function declaration");
      }
      allocator_free(ctx->allocator, id);
      curr = value_clone(curr, ctx->allocator);
      context_declar(ctx, NULL, curr);
      return curr;
    } else {
      value_t func = create_function(ctx, function_type, node, id);
      allocator_free(ctx->allocator, id);
      return func;
    }
  onerror:
    allocator_free(ctx->allocator, args);
    allocator_free(ctx->allocator, return_type);
    if (err) {
      err = value_clone(err, ctx->allocator);
    }
    ctx->current = current_scope;
    allocator_free(ctx->allocator, scope);
    context_declar(ctx, NULL, err);
    return err;
  }
  value_t func = create_template(ctx, node);
  for (size_t idx = 0; idx < ast_get_length(closure); idx++) {
    ast_node_t item = ast_get_item(closure, idx);
    if (item->type == NODE_TYPE_LITERAL_IDENTIFIER) {
      char *name = location_get(node_get_location(item), ctx->allocator);
      value_t value = context_load(ctx, name);
      if (value->type->kind == TYPE_KIND_ERROR) {
        allocator_free(ctx->allocator, name);
        return value;
      }
      value_t err = function_add_closure(func, ctx, name, value);
      allocator_free(ctx->allocator, name);
      if (err->type->kind == TYPE_KIND_ERROR) {
        return convert_comptime_error(ctx, node_get_location(item), err);
      }
    }
  }
  value_t ins = template_create_default_instance(func, ctx);
  if (ins) {
    func = ins;
  }
  if (ast_get_length(closure) && func->type->kind == TYPE_KIND_FUNCTION &&
      !ctx->comptime) {
    ast_node_t bind = create_ast_value(ctx->allocator, func);
    ast_add_child(ctx->allocator, node, "bind", bind);
    return context_create_value(ctx, func->type, false, NULL);
  }
  return func;
}