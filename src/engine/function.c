#include "engine/function.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/hash_map.h"
#include "core/location.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/module.h"
#include "engine/type.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static void function_declar_dispose(function_declar_t self,
                                    allocator_t allocator) {
  allocator_free(allocator, self->id);
}

function_declar_t create_function_declar(allocator_t allocator, const char *id,
                                         ast_node_t node, type_t self,
                                         type_t global) {
  function_declar_t declar =
      allocator_alloc(allocator, sizeof(struct _function_declar_t),
                      (dispose_fn_t)function_declar_dispose);
  declar->self = self;
  declar->global = global;
  declar->id = create_cstring(allocator, id);
  declar->node = node;
  declar->kind = FUNCTION_KIND_NORMAL;
  declar->closure =
      create_hash_map(allocator, &(hash_map_initialize_t){
                                     .autofree_key = true,
                                     .autofree_value = true,
                                     .hash = (hash_fn_t)cstring_sdb,
                                     .compare = (compare_fn_t)strcmp,
                                 });
  return declar;
}

typedef struct _function_meta_t *function_meta_t;
struct _function_meta_t {
  ctype_t type;
  array_t args;
  bool variadic;
};
static void function_meta_dispsoe(function_meta_t self, allocator_t allocator) {
  allocator_free(allocator, self->type);
  allocator_free(allocator, self->args);
}
static function_meta_t create_function_meta(allocator_t allocator, ctype_t type,
                                            array_t args, bool variadic) {
  function_meta_t self =
      allocator_alloc(allocator, sizeof(struct _function_meta_t),
                      (dispose_fn_t)function_meta_dispsoe);
  self->type = create_ctype(allocator, type->type, type->mut);
  self->variadic = variadic;
  self->args = create_array(allocator, &(array_initialize_t){.autofree = true});
  for (size_t idx = 0; idx < array_get_size(args); idx++) {
    ctype_t arg = array_get(args, idx);
    array_push(self->args, create_ctype(allocator, arg->type, arg->mut));
  }
  return self;
}

static value_t function_call(value_t self, context_t ctx, size_t argc,
                             value_t *argv) {
  function_declar_t declar = *(function_declar_t *)self->data;
  if (declar->kind == FUNCTION_KIND_NATIVE) {
    return declar->handle(ctx, argc, argv);
  }
  return create_error(ctx, "not implement");
}

type_t create_function_type(context_t ctx, ctype_t type, array_t argv,
                            bool variadic) {
  size_t len = 0;
  len = 1; // F
  if (!type->mut) {
    len += 1; // C
  }
  len += strlen(type->type->id);
  for (size_t idx = 0; idx < array_get_size(argv); idx++) {
    ctype_t arg = array_get(argv, idx);
    len++; // A
    len += strlen(arg->type->id);
    if (!arg->mut) {
      len++; // C
    }
  }
  if (variadic) {
    len += 1; // V;
  }
  char id[len + 1];
  size_t offset = 0;
  id[offset++] = 'F';
  strcpy(&id[offset], type->type->id);
  if (!type->mut) {
    id[offset++] = 'C';
  }
  offset += strlen(type->type->id);
  for (size_t idx = 0; idx < array_get_size(argv); idx++) {
    ctype_t arg = array_get(argv, idx);
    id[offset++] = 'A';
    strcpy(&id[offset], arg->type->id);
    offset += strlen(arg->type->id);
    if (!arg->mut) {
      id[offset++] = 'C';
    }
  }
  id[offset] = 0;
  type_t func_type = context_load_type(ctx, id);
  if (!func_type) {
    size_t len = strlen("func(");
    for (size_t idx = 0; idx < array_get_size(argv); idx++) {
      ctype_t arg = array_get(argv, idx);
      if (idx != 0) {
        len += 1;
      }
      if (idx == array_get_size(argv) - 1 && variadic) {
        len += 3;
      }
      if (arg->type) {
        if (!arg->mut) {
          len += strlen("const ");
        }
        len += strlen(arg->type->name);
      }
    }
    len += 2;
    if (type->mut) {
      len += strlen("const ");
    }
    len += strlen(type->type->name);
    len++;
    char name[len];
    size_t offset = 0;
    strcpy(&name[offset], "func(");
    offset += strlen("func(");
    for (size_t idx = 0; idx < array_get_size(argv); idx++) {
      ctype_t arg = array_get(argv, idx);
      if (idx != 0) {
        name[offset++] = ',';
      }
      if (idx == array_get_size(argv) - 1 && variadic) {
        strcpy(&name[offset], "...");
        offset += 3;
      }
      if (arg->type) {
        if (!arg->mut) {
          strcpy(&name[offset], "const ");
          offset += strlen("const ");
        }
        strcpy(&name[offset], arg->type->name);
        offset += strlen(arg->type->name);
      }
    }
    name[offset++] = ')';
    name[offset++] = ':';
    if (!type->mut) {
      strcpy(&name[offset], "const ");
      offset += strlen("const ");
    }
    strcpy(&name[offset], type->type->name);
    offset += strlen(type->type->name);
    name[offset] = 0;
    struct _type_operator_t opt = {
        .call = function_call,
    };
    function_meta_t meta =
        create_function_meta(ctx->allocator, type, argv, variadic);
    func_type = create_type(ctx->allocator, TYPE_KIND_FUNCTION, name, id,
                            sizeof(void *), sizeof(void *), &opt, meta);
    context_store_type(ctx, func_type);
  }
  return func_type;
}
value_t create_function(context_t ctx, type_t type, ast_node_t node) {
  size_t len = 0;
  const char *parent_id = NULL;
  if (ctx->type == CONTEXT_TYPE_STRUCT) {
    parent_id = ctx->self->id;
  } else if (ctx->type == CONTEXT_TYPE_FUNCTION) {
    function_declar_t declar = *(function_declar_t *)ctx->function->data;
    parent_id = declar->id;
  }
  len = strlen(parent_id);
  len++;
  ast_node_t identifier = ast_get_child(node, "identifier");
  if (identifier) {
    len +=
        identifier->start->loc.end.offset - identifier->start->loc.begin.offset;
  }
  char base_id[len + 1];
  if (identifier) {
    char *name = location_get(node_get_location(identifier), ctx->allocator);
    sprintf(base_id, "%s_%s", parent_id, name);
    allocator_free(ctx->allocator, name);
  } else {
    sprintf(base_id, "%s_nonamed", parent_id);
  }
  char *id = NULL;
  module_t mod = ctx->mod;
  if (hash_map_has(mod->functions, base_id, NULL, NULL)) {
    for (size_t idx = 0;; idx++) {
      size_t len = snprintf(NULL, 0, "%sI%" PRIuPTR, base_id, idx);
      char buf[len + 1];
      sprintf(buf, "%sI%" PRIuPTR, base_id, idx);
      if (!hash_map_has(mod->functions, buf, NULL, NULL)) {
        id = create_cstring(ctx->allocator, buf);
        break;
      }
    }
  } else {
    id = create_cstring(ctx->allocator, base_id);
  }
  function_declar_t declar =
      create_function_declar(ctx->allocator, id, node, ctx->self, ctx->global);
  allocator_free(ctx->allocator, id);
  array_push(ctx->functions, declar);
  value_t value =
      context_create_comptime_value(ctx, type, &declar, false, NULL);
  value_t val = value_clone(value, ctx->allocator);
  array_push(mod->indexed_functions, val);
  hash_map_set(mod->functions, declar->id, val, NULL, NULL);
  return value;
}