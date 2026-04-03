#ifndef _H_CUBEC_ENGINE_MODULE_
#define _H_CUBEC_ENGINE_MODULE_
#include "ast/node.h"
#include "core/allocator.h"
#include "engine/value.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_module_t *cubec_module_t;
cubec_module_t cubec_create_module(cubec_allocator_t allocator,
                                   cubec_ast_node_t node, const char *filename,
                                   cubec_value_t value);
const char *cubec_module_get_filename(cubec_module_t self);
const char *cubec_module_get_dirname(cubec_module_t self);
cubec_ast_node_t cubec_module_get_node(cubec_module_t self);
cubec_value_t cubec_module_get_value(cubec_module_t self);
#ifdef __cplusplus
}
#endif
#endif