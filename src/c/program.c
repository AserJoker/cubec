#include "c/program.h"
#include "c/type.h"
#include "core/hash_map.h"
#include "core/stream.h"
#include "engine/type.h"

void c_program(context_t ctx, ast_node_t node, stream_t stream) {
  stream_write(stream, "#include <stdbool.h>");
  stream_newline(stream);
  stream_write(stream, "#include <stdint.h>");
  stream_newline(stream);
  stream_write(stream, "typedef _Float16 float16_t;");
  stream_newline(stream);
  stream_write(stream, "typedef float float32_t;");
  stream_newline(stream);
  stream_write(stream, "typedef double float64_t;");
  stream_newline(stream);
  list_node_t it = hash_map_get_first(ctx->types);
  while (it != hash_map_get_end(ctx->types)) {
    type_t type = hash_map_node_get_value(it);
    c_type_declarator(ctx, type, stream);
    it = hash_map_node_get_next(it);
  }
  it = hash_map_get_first(ctx->types);
  while (it != hash_map_get_end(ctx->types)) {
    type_t type = hash_map_node_get_value(it);
    c_type_declaration(ctx, type, stream);
    it = hash_map_node_get_next(it);
  }
}