#include "c/program.h"
#include "ast/node.h"
#include "c/writer.h"
#include "core/array.h"
#include "core/hash_map.h"
#include "core/list.h"
#include "engine/context.h"
#include <string.h>

void c_program(c_writer_t writer, ast_node_t node) {
  context_t ctx = writer->ctx;
  ast_node_t statements = ast_get_child(node, "statements");
  for (size_t idx = 0; idx < ast_get_length(statements); idx++) {
    ast_node_t sts = ast_get_item(statements, idx);
  }
  hash_map_t functions = ctx->mod->functions;
  list_node_t it = hash_map_get_first(functions);
  while (it != hash_map_get_end(functions)) {
    value_t func = hash_map_node_get_value(it);
    array_push(writer->functions, func);
    it = hash_map_node_get_next(it);
  }
}