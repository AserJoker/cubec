#include "eval/statement_test.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/map.h"
#include "engine/context.h"
#include "engine/scope.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/statement_block.h"
#include <stdio.h>

cubec_value_t cubec_eval_statement_test(cubec_context_t ctx,
                                        cubec_ast_node_t sts,
                                        const char *filename) {
  cubec_eval_mode_t mode = ctx->eval_mode;
  ctx->eval_mode = CUBEC_EVAL_TEST;
  cubec_ast_node_t name_node = cubec_map_get(sts->children, "name", NULL);
  cubec_ast_node_t body_node = cubec_map_get(sts->children, "body", NULL);
  char *name = cubec_location_get_str(name_node->loc, ctx->allocator);
  fprintf(stdout, "=================================\n");
  fprintf(stdout, "test %s start\n", name);
  cubec_scope_t current = ctx->current;
  cubec_value_t err = cubec_eval_statement_block(ctx, body_node, filename);
  if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
    fprintf(stdout, "test %s failed\n", name);
    fprintf(stderr, "%s\n", *(const char **)err->data);
    while (current != ctx->root) {
      cubec_context_pop_scope(ctx);
    }
  } else {
    fprintf(stdout, "test %s finish\n", name);
  }
  fprintf(stdout, "=================================\n");
  cubec_allocator_free(ctx->allocator, name);
  ctx->eval_mode = mode;
  return ctx->value_undefined;
}