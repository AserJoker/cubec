#ifndef _H_C_WRITER_
#define _H_C_WRITER_
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
  hash_map_t global_values;
  hash_map_t modules;
  stream_t stream;
};
c_writer_t create_c_writer(context_t ctx, stream_t stream);
void c_writer_add_type(c_writer_t writer, type_t type);
void c_writer_add_function(c_writer_t writer, value_t function);
void c_writer_add_global(c_writer_t writer, const char *name, value_t global);
void c_writer_import(c_writer_t writer, const char *src);
void c_writer_write(c_writer_t writer);
#ifdef __cplusplus
}
#endif
#endif