#ifndef _H_CUBEC_ENGINE_CONTEXT_
#define _H_CUBEC_ENGINE_CONTEXT_
#include "ast/node.h"
#include "core/allocator.h"
#include "engine/scope.h"
#include "engine/value.h"
#include "engine/variable.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cubec_context_t *cubec_context_t;
struct _cubec_context_t {
  cubec_scope_t root;
  cubec_scope_t current;
  cubec_allocator_t allocator;
  cubec_value_t struct_type;
  cubec_value_t field_type;
  cubec_value_t method_type;
  cubec_value_t array_type;
  cubec_value_t enum_type;
};

cubec_context_t cubec_create_context(cubec_allocator_t allocator);
void cubec_context_push_scope(cubec_context_t self);
void cubec_context_pop_scope(cubec_context_t self);
cubec_variable_t cubec_context_create_variable(cubec_context_t self,
                                               bool mutable,
                                               cubec_value_t value, char *name);
cubec_variable_t cubec_context_load(cubec_context_t self, const char *name);
cubec_variable_t cubec_context_run(cubec_context_t self, cubec_ast_node_t node);

#ifdef __cplusplus
}
#endif
#endif