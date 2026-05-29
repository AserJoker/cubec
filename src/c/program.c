#include "c/program.h"
#include "core/stream.h"

static void c_program_content(context_t ctx, ast_node_t node, stream_t stream) {
  
}

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
  c_program_content(ctx, node, stream);
}