#include "c/variable_declarator.h"
#include "ast/node.h"
#include "c/type.h"
#include "core/stream.h"
#include "engine/array.h"
#include "engine/context.h"
#include "engine/type.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

void write_c_declar(context_t ctx, type_t type, ast_node_t identifier,
                    stream_t stream);

void write_c_array_declar(context_t ctx, type_t type, ast_node_t identifier,
                          stream_t stream) {
  type_t base_type = array_type_get_type(type);
  size_t len = array_type_get_length(type);
  write_c_declar(ctx, base_type, identifier, stream);
  size_t size = snprintf(NULL, 0, "[%" PRIuPTR "]", len);
  char buf[size];
  sprintf(buf, "[%" PRIuPTR "]", len);
  stream_write(stream, buf);
}

void write_c_declar(context_t ctx, type_t type, ast_node_t identifier,
                    stream_t stream) {
  if (type_get_kind(type) == TYPE_KIND_ARRAY) {
    write_c_array_declar(ctx, type, identifier, stream);
  } else if (type_get_kind(type) == TYPE_KIND_FUNCTION) {

  } else {
    write_c_type(ctx, type, stream);
    stream_write(stream, " ");
    stream_write_location(stream, identifier->loc);
  }
}
