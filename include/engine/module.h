#ifndef _H_ENGINE_MODULE_
#define _H_ENGINE_MODULE_
#include "ast/node.h"
#include "core/allocator.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
struct _value_t;
typedef struct _module_t *module_t;
module_t create_module(allocator_t allocator, value_t value, ast_node_t node,
                       char *source, const char *filename);
value_t module_get_value(module_t self);
const char *module_get_filename(module_t self);
const char *module_get_dirname(module_t self);
ast_node_t module_get_node(module_t self);
void module_add_function(module_t self, struct _value_t *func);
void module_add_struct(module_t self, struct _value_t *str);
struct _value_t *module_get_function(module_t self, const char *id);
struct _value_t *module_get_struct(module_t self, const char *id);
array_t module_get_functions(module_t self);
array_t module_get_structs(module_t self);
array_t module_get_errors(module_t self);
void module_add_error(module_t self, value_t err);
char *module_generator_func_id(module_t self, allocator_t allocator,
                               const char *base_id);
#ifdef __cplusplus
}
#endif
#endif