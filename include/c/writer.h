#ifndef _H_C_WRITER_
#define _H_C_WRITER_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/hash_map.h"
#include "core/stream.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _c_writer_t *c_writer_t;
struct _c_writer_t {
  context_t ctx;
  array_t types;
  array_t functions;
  array_t globals;
  hash_map_t modules;
  stream_t stream;
};
typedef struct _c_global_t *c_global_t;
struct _c_global_t {
  char *name;
  ast_node_t initialize;
  type_t type;
  bool mut;
};
c_global_t create_c_global(allocator_t allocator, const char *name,
                           ast_node_t initialize, type_t type, bool mut);
c_writer_t create_c_writer(context_t ctx, stream_t stream);
void c_writer_add_type(c_writer_t writer, type_t type);
void c_writer_add_function(c_writer_t writer, value_t function);
void c_writer_add_global(c_writer_t writer, const char *name,
                         ast_node_t initialize, type_t type, bool mut);
void c_writer_write(c_writer_t writer);
#ifdef __cplusplus
}
#endif
#endif