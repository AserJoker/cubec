#include "c/literal_identifier.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/hash_map.h"
#include "core/location.h"
#include "core/position.h"
#include "core/stream.h"
#include "engine/context.h"
#include "engine/function.h"
void c_literal_identifier(c_writer_t writer, ast_node_t node) {
  stream_t stream = writer->stream;
  context_t ctx = writer->ctx;
  allocator_t allocator = ctx->allocator;
  char *id = location_get(node_get_location(node), allocator);
  if (context_load_local(ctx, id)) {
    stream_write(stream, id);
  } else {
    function_declar_t declar = NULL;
    if (ctx->function) {
      declar = *(function_declar_t *)ctx->function->data;
    }
    if (declar && hash_map_has(declar->closure, id, NULL, NULL)) {
      stream_write(stream, "(__env__->%s)", id);
    } else {
      stream_write(stream, "%s_%s", ctx->global->id, id);
    }
  }
  allocator_free(allocator, id);
}