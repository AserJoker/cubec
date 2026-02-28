#include "engine/context.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/map.h"
#include "core/string.h"
#include "engine/scope.h"
#include "engine/value.h"
#include "engine/variable.h"
#include <stdbool.h>
#include <string.h>

static void cubec_context_dispose(cubec_context_t self,
                                  cubec_allocator_t allocator) {
  while (self->current) {
    cubec_context_pop_scope(self);
  }
}

static void cubec_context_init_global(cubec_context_t ctx) {
  {
    cubec_value_t struct_type =
        cubec_create_struct_value(ctx->allocator, true, NULL);
    cubec_struct_data_t struct_data = struct_type->data;
    struct_data->type = struct_type;
    ctx->struct_type = struct_type;
    /**
     * struct Struct {
     *  fields:[]Field;
     *  methods:[]Method;
     *  attributes:[]Field;
     *  name:str;
     * };
     */
    cubec_value_t name = cubec_create_str_value(
        ctx->allocator, true, cubec_create_cstring(ctx->allocator, "Struct"));
    cubec_map_set(struct_data->fields, ctx->allocator,
                  cubec_create_cstring(ctx->allocator, "name"), name, NULL);
  }
  {
  }
}

cubec_context_t cubec_create_context(cubec_allocator_t allocator) {
  cubec_context_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_context_t),
                            (cubec_dispose_fn_t)cubec_context_dispose);
  self->allocator = allocator;
  self->root = cubec_create_scope(allocator, NULL);
  self->current = self->root;
  cubec_context_init_global(self);
  return self;
}
void cubec_context_push_scope(cubec_context_t self) {
  cubec_scope_t scope = cubec_create_scope(self->allocator, self->current);
  self->current = scope;
}
void cubec_context_pop_scope(cubec_context_t self) {
  cubec_scope_t parent = self->current->parent;
  cubec_list_node_t it = cubec_list_get_last(self->current->defers);
  while (it != cubec_list_get_begin(self->current->defers)) {
    // TODO: run defer
    it = cubec_list_node_next(it);
  }
  cubec_allocator_free(self->allocator, self->current);
  self->current = parent;
}
cubec_variable_t cubec_context_create_variable(cubec_context_t self,
                                               bool mutable,
                                               cubec_value_t value,
                                               char *name) {
  cubec_variable_t variable =
      cubec_create_varaible(self->allocator, value, mutable);
  cubec_list_append(self->current->variables, self->allocator, variable);
  if (name) {
    cubec_map_set(self->current->named_variables, self->allocator, name,
                  variable, NULL);
  }
  return variable;
}
cubec_variable_t cubec_context_load(cubec_context_t self, const char *name) {
  cubec_scope_t scope = self->current;
  while (scope) {
    cubec_variable_t variable =
        cubec_map_get(scope->named_variables, name, NULL);
    if (variable) {
      return variable;
    }
    scope = scope->parent;
  }
  return NULL;
}
cubec_variable_t cubec_context_run(cubec_context_t self,
                                   cubec_ast_node_t node) {
  return cubec_context_load(self, "undefined");
}