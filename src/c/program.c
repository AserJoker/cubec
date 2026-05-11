#include "c/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "c/statement_declaration.h"
#include "c/type.h"
#include "core/array.h"
#include "core/stream.h"
#include "engine/context.h"
#include "engine/module.h"
#include "engine/type.h"
void write_c_program(context_t ctx, ast_node_t node, stream_t stream) {
  ast_node_t statements = ast_get_child(node, "statements");
  stream_write(stream, "#include <stdbool.h>\n");
  stream_write(stream, "#include <stdint.h>\n");
  stream_write(stream, "#include <stdlib.h>\n");
  module_t module = context_get_module(ctx);
  array_t types = module_get_types(module);
  for (size_t idx = 0; idx < array_get_size(types); idx++) {
    type_t type = array_get(types, idx);
    if (type_get_kind(type) != TYPE_KIND_FUNCTION) {
      write_c_type_declarator(ctx, type, stream);
      stream_newline(stream);
    }
  }
  for (size_t idx = 0; idx < array_get_size(types); idx++) {
    type_t type = array_get(types, idx);
    if (type_get_kind(type) == TYPE_KIND_FUNCTION) {
      write_c_type_declaration(ctx, type, stream);
      stream_newline(stream);
    }
  }
  for (size_t idx = 0; idx < array_get_size(types); idx++) {
    type_t type = array_get(types, idx);
    if (type_get_kind(type) != TYPE_KIND_FUNCTION) {
      write_c_type_declaration(ctx, type, stream);
      stream_newline(stream);
    }
  }
  for (size_t idx = 0; idx < ast_get_length(statements); idx++) {
    ast_node_t sts = ast_get_item(statements, idx);
    if (sts->visible) {
      if (sts->type == NODE_TYPE_STATEMENT_DECLARATION) {
        write_c_statement_declaration(ctx, sts, stream);
      }
      stream_newline(stream);
    }
  }
}