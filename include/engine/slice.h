#ifndef _H_ENGINE_SLICE_
#define _H_ENGINE_SLICE_
#include "engine/context.h"
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _slice_data_t *slice_data_t;
struct _slice_data_t {
  void *data;
  size_t offset;
  size_t length;
};
type_t create_slice_type(context_t ctx, type_t type);
type_t slice_type_get_type(type_t type);
value_t create_comptime_slice(context_t ctx, type_t type, void *data,
                              size_t offset, size_t length, bool mut,
                              const char *name);
#ifdef __cplusplus
}
#endif
#endif