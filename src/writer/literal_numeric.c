#include "writer/literal_numeric.h"
void cubec_write_literal_numeric(FILE *fp, cubec_ast_node_t node,
                                 cubec_write_context *ctx) {
  char *c_identifier = cubec_location_get(node->loc, ctx->allocator);
  fprintf(fp, "%s", c_identifier);
  cubec_allocator_free(ctx->allocator, c_identifier);
}