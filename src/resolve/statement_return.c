#include "resolve/statement_return.h"
#include "ast/node.h"
#include "engine/error.h"
#include "engine/function.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/void.h"
#include "resolve/expression.h"

value_t resolve_statement_return(context_t ctx, ast_node_t node) {
  ast_node_t value = ast_get_child(node, "value");
  value_t val = NULL;
  if (value) {
    val = resolve_expression(ctx, value);
    if (val->type->kind == TYPE_KIND_ERROR) {
      return val;
    }
  } else {
    val = create_comptime_void(ctx);
  }

  value_t func = ctx->function;
  function_meta_t meta = func->type->meta;
  val = value_safe_convert(val, ctx, meta->type->type);
  if (val->type->kind == TYPE_KIND_ERROR) {
    return convert_comptime_error(ctx, node_get_location(node), val);
  }
  if (val->type->kind == TYPE_KIND_PTR || val->type->kind == TYPE_KIND_SLICE) {
    if (meta->type->mut && !val->mut) {
      return create_comptime_error(ctx, node_get_location(value),
                                   "cannot convet const ptr to non-const ptr");
    }
  }
  if (ctx->comptime) {
    return val;
  } else {
    return val;
  }
}