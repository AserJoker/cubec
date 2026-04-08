#include "writer/initialize_field.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "writer/expression.h"
void cubec_write_initialize_field(FILE *fp, cubec_ast_node_t node,
                                  cubec_write_context *ctx) {
  cubec_ast_node_t identifier = cubec_ast_get_child(node, "identifier");
  cubec_ast_node_t initialize = cubec_ast_get_child(node, "initialize");
  if (identifier) {
    char *c_id = cubec_location_get(identifier->loc, ctx->allocator);
    fprintf(fp, ".%s = ", c_id);
    cubec_allocator_free(ctx->allocator, c_id);
  }
  cubec_write_expression(fp, initialize, ctx);
}