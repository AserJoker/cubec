#include "eval/type.h"
#include "ast/node.h"
#include "core/map.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"

cubec_value_t cubec_eval_type(cubec_context_t ctx, cubec_ast_node_t type,
                              const char *filename) {
  cubec_ast_node_t node = cubec_map_get(type->children, "expression", NULL);
  cubec_value_t val = cubec_eval_expression(ctx, node, filename);
  if (val->type->kind != CUBEC_TYPE_KIND_TYPE) {
    return cubec_context_create_compile_error(ctx, node, filename,
                                              "Invalid type");
  }
  return val;
}