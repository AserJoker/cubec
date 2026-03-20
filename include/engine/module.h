#ifndef _H_CUBEC_ENGINE_MODULE_
#define _H_CUBEC_ENGINE_MODULE_
#include "core/allocator.h"
#include "core/array.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_module_t *cubec_module_t;
struct _cubec_module_t {
  cubec_array_t functions;
  cubec_array_t structs;
  cubec_array_t enums;
  cubec_array_t variables;
  char *filename;
  char *dirname;
};
cubec_module_t cubec_create_module(cubec_allocator_t allocator,
                                   const char *filename, const char *dirname);
#ifdef __cplusplus
}
#endif
#endif