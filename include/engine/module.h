#ifndef _H_CUBEC_ENGINE_MODULE_
#define _H_CUBEC_ENGINE_MODULE_
#include "core/allocator.h"
#include "core/map.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_module_t *cubec_module_t;
struct _cubec_module_t {
  cubec_map_t exports;
  const char *dirname;
  const char *filename;
};
cubec_module_t cubec_create_module(cubec_allocator_t allocator,
                                   const char *dirname, const char *filename);

#ifdef __cplusplus
}
#endif
#endif