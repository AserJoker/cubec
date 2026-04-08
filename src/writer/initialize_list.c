#include "writer/initialize_list.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "writer/expression.h"
#include "writer/initialize_field.h"

void cubec_write_initialize_list(FILE *fp, cubec_ast_node_t node,
                                 cubec_write_context *ctx) {
  cubec_ast_node_t type = cubec_ast_get_child(node, "type");
  cubec_write_expression(fp, type, ctx);
  cubec_ast_node_t fields = cubec_ast_get_child(node, "fields");
  if (cubec_ast_get_length(fields) <= 1) {
    fprintf(fp, "{");
    for (size_t idx = 0; idx < cubec_ast_get_length(fields); idx++) {
      cubec_ast_node_t field = cubec_ast_get_item(fields, idx);
      if (field->type == CUBEC_NODE_TYPE_INITIALIZE_FIELD) {
        cubec_write_initialize_field(fp, field, ctx);
      } else {
        cubec_write_expression(fp, field, ctx);
      }
    }
    fprintf(fp, "}");
  } else {
    fprintf(fp, "{");
    ctx->indent++;
    fprintf(fp, "\n");
    for (size_t idx = 0; idx < cubec_ast_get_length(fields); idx++) {
      if (ctx->indent) {
        fprintf(fp, "%*s", (int)ctx->indent * 4, " ");
      }
      cubec_ast_node_t field = cubec_ast_get_item(fields, idx);
      if (field->type == CUBEC_NODE_TYPE_INITIALIZE_FIELD) {
        cubec_write_initialize_field(fp, field, ctx);
      } else {
        cubec_write_expression(fp, field, ctx);
      }
      if (idx != cubec_ast_get_length(fields) - 1) {
        fprintf(fp, ",\n");
      }
    }
    fprintf(fp, "\n");
    ctx->indent--;
    if (ctx->indent) {
      fprintf(fp, "%*s", (int)ctx->indent * 4, " ");
    }
    fprintf(fp, "}");
  }
}