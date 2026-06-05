#include "c/statement_function.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "c/function.h"
#include "engine/function.h"
#include <string.h>

void c_statement_function(c_writer_t writer, ast_node_t node) {
  ast_node_t function = ast_get_child(node, "function");
  if (function->type == NODE_TYPE_FUNCTION_DECLARATOR) {
    stream_t stream = writer->stream;
    context_t ctx = writer->ctx;
    allocator_t allocator = ctx->allocator;
    ast_node_t bind = ast_get_child(function, "bind");
    value_t function = bind->value;
    function_declar_t declar = *(function_declar_t *)function->data;
    if (function->type->kind == TYPE_KIND_TEMPLATE) {
      list_node_t it = hash_map_get_first(ctx->functions);
      while (it != hash_map_get_end(ctx->functions)) {
        value_t func = hash_map_node_get_value(it);
        function_declar_t fdeclar = *(function_declar_t *)func->data;
        if (strcmp(declar->id, fdeclar->template_id) == 0) {
          stream_write(stream, "%s %s = ", func->type->id, fdeclar->id);
          c_function_closure(writer, func);
          stream_write(stream, ";");
          stream_newline(stream);
        }
        it = hash_map_node_get_next(it);
      }
    } else {
      stream_write(stream, "%s %s = ", function->type->id, declar->id);
      c_function_closure(writer, bind->value);
      stream_write(stream, ";");
      stream_newline(stream);
    }
  }
}