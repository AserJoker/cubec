#include "c/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "c/function_declarator.h"
#include "c/statement_declaration.h"
#include "c/type.h"
#include "core/array.h"
#include "core/stream.h"
#include "engine/context.h"
#include "engine/module.h"
#include "engine/type.h"
void write_c_program(context_t ctx, ast_node_t node, stream_t stream) {
  ast_node_t statements = ast_get_child(node, "statements");
  stream_write(stream, "#include <stdbool.h>");
  stream_newline(stream);
  stream_write(stream, "#include <stdint.h>");
  stream_newline(stream);
  stream_write(stream, "#include <stdlib.h>");
  stream_newline(stream);
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
  array_t functions = module_get_functions(module);
  for (size_t idx = 0; idx < array_get_size(functions); idx++) {
    value_t func = array_get(functions, idx);
    write_c_function_declarator(ctx, func, stream);
  }
  for (size_t idx = 0; idx < ast_get_length(statements); idx++) {
    ast_node_t sts = ast_get_item(statements, idx);
    if (sts->visible) {
      if (sts->type == NODE_TYPE_STATEMENT_DECLARATION) {
        write_c_statement_declaration(ctx, sts, stream);
        stream_newline(stream);
      }
    }
  }
  for (size_t idx = 0; idx < array_get_size(functions); idx++) {
    value_t func = array_get(functions, idx);
    write_c_function_declaration(ctx, func, stream);
  }
}