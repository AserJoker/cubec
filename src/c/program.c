#include "c/program.h"
#include "ast/node.h"
#include "c/writer.h"
#include "core/hash_map.h"
#include "core/list.h"
#include "engine/context.h"
#include "engine/function.h"

void c_program(c_writer_t writer, ast_node_t node) {
  context_t ctx = writer->ctx;
  type_t stru = *(type_t *)ctx->mod->value->data;
  c_writer_add_type(writer, stru);
  hash_map_t functions = ctx->mod->functions;
  list_node_t it = hash_map_get_first(functions);
  while (it != hash_map_get_end(functions)) {
    value_t func = hash_map_node_get_value(it);
    function_declar_t declar = *(function_declar_t *)func->data;
    if (declar->kind != FUNCTION_KIND_COMPTIME) {
      c_writer_add_function(writer, func);
    }
    it = hash_map_node_get_next(it);
  }
}