#include "engine/function.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/hash_map.h"
#include "core/list.h"
#include "core/location.h"
#include "core/string.h"
#include "engine/arr.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/module.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include "engine/void.h"
#include "resolve/type.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static value_t template_call(value_t self, context_t ctx, size_t argc,
                             value_t *argv) {
  value_t func = template_create_instance(self, ctx, argc, argv);
  if (func->type->kind == TYPE_KIND_ERROR) {
    return func;
  }
  return value_call(func, ctx, argc, argv);
}

void init_template_type(context_t ctx) {
  struct _type_operator_t opt = {
      .call = &template_call,
  };
  type_t type = create_type(ctx->allocator, TYPE_KIND_TEMPLATE, "template",
                            "template", sizeof(function_declar_t),
                            sizeof(function_declar_t), &opt, NULL);
  context_store_type(ctx, type);
}
static void function_declar_dispose(function_declar_t self,
                                    allocator_t allocator) {
  allocator_free(allocator, self->id);
  allocator_free(allocator, self->closure);
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

static bool function_type_is_equal(type_t self, type_t another) {
  function_meta_t self_meta = self->meta;
  function_meta_t another_meta = another->meta;
  if (self_meta->variadic != another_meta->variadic) {
    return false;
  }
  if (self_meta->type->mut != another_meta->type->mut) {
    return false;
  }
  if (!type_is_equal(self_meta->type->type, another_meta->type->type)) {
    return false;
  }
  if (array_get_size(self_meta->args) != array_get_size(another_meta->args)) {
    return false;
  }
  for (size_t idx = 0; idx < array_get_size(self_meta->args); idx++) {
    ctype_t self_arg = array_get(self_meta->args, idx);
    ctype_t another_arg = array_get(another_meta->args, idx);
    if (self_arg->mut != another_arg->mut) {
      return false;
    }
    if (!type_is_equal(self_arg->type, another_arg->type)) {
      return false;
    }
  }
  return true;
}

type_t create_function_type(context_t ctx, ctype_t type, array_t argv,
                            bool variadic) {
  size_t len = 0;
  len = 1; // F
  len++;   // R
  if (type->type) {
    len += strlen(type->type->id);
  }
  if (!type->mut) {
    len += 1; // C
  }
  for (size_t idx = 0; idx < array_get_size(argv); idx++) {
    ctype_t arg = array_get(argv, idx);
    len++; // A
    if (arg->type) {
      len += strlen(arg->type->id);
    }
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
  id[offset++] = 'R';
  if (type->type) {
    strcpy(&id[offset], type->type->id);
    offset += strlen(type->type->id);
  }
  if (!type->mut) {
    id[offset++] = 'C';
  }
  for (size_t idx = 0; idx < array_get_size(argv); idx++) {
    ctype_t arg = array_get(argv, idx);
    id[offset++] = 'A';
    if (arg->type) {
      strcpy(&id[offset], arg->type->id);
      offset += strlen(arg->type->id);
    }
    if (idx == array_get_size(argv) - 1 && variadic) {
      if (variadic) {
        id[offset++] = 'V';
      }
    }
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
    if (type->type) {
      len += strlen(type->type->name);
      len += 2;
    }
    char name[len];
    size_t offset = 0;
    strcpy(&name[offset], "func(");
    offset += strlen("func(");
    for (size_t idx = 0; idx < array_get_size(argv); idx++) {
      ctype_t arg = array_get(argv, idx);
      if (idx != 0) {
        name[offset++] = ',';
      }
      if (!arg->mut) {
        strcpy(&name[offset], "const ");
        offset += strlen("const ");
      }
      if (idx == array_get_size(argv) - 1 && variadic) {
        strcpy(&name[offset], "...");
        offset += 3;
      }
      if (arg->type) {
        strcpy(&name[offset], arg->type->name);
        offset += strlen(arg->type->name);
      }
    }
    name[offset++] = ')';
    name[offset++] = '-';
    name[offset++] = '>';
    if (!type->mut) {
      strcpy(&name[offset], "const ");
      offset += strlen("const ");
    }
    if (type->type) {
      strcpy(&name[offset], type->type->name);
      offset += strlen(type->type->name);
    } else {
      name[offset++] = '?';
    }
    name[offset] = 0;
    struct _type_operator_t opt = {
        .call = function_call,
        .type_equal = function_type_is_equal,
    };
    function_meta_t meta =
        create_function_meta(ctx->allocator, type, argv, variadic);
    func_type = create_type(ctx->allocator, TYPE_KIND_FUNCTION, name, id,
                            sizeof(void *), sizeof(void *), &opt, meta);
    context_store_type(ctx, func_type);
  }
  return func_type;
}
value_t create_function(context_t ctx, type_t type, ast_node_t node,
                        const char *base_id) {
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
value_t function_add_closure(value_t self, context_t ctx, const char *name,
                             value_t value) {
  function_declar_t declar = *(function_declar_t *)self->data;
  if (hash_map_has(declar->closure, name, NULL, NULL)) {
    return create_error(ctx, "duplicate '%s' closure declar", name);
  }
  hash_map_set(declar->closure, create_cstring(ctx->allocator, name),
               value_clone(value, ctx->allocator), NULL, NULL);
  return create_comptime_void(ctx);
}

value_t create_template(context_t ctx, ast_node_t node) {
  type_t type = context_load_type(ctx, "template");

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
  array_push(ctx->functions, declar);
  allocator_free(ctx->allocator, id);
  return context_create_comptime_value(ctx, type, &declar, false, NULL);
}
value_t template_create_instance(value_t self, context_t ctx, size_t argc,
                                 value_t *argv) {

  function_declar_t declar = *(function_declar_t *)self->data;
  ast_node_t node = declar->node;
  ast_node_t arguments = ast_get_child(node, "arguments");
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t mut = ast_get_child(node, "mut");
  value_t err = NULL;
  ctype_t return_type = NULL;
  array_t args = create_array(ctx->allocator, &(array_initialize_t){
                                                  .autofree = true,
                                              });
  context_push_scope(ctx);
  list_node_t it = hash_map_get_first(declar->closure);
  while (it != hash_map_get_end(declar->closure)) {
    const char *key = hash_map_node_get_key(it);
    value_t value = hash_map_node_get_value(it);
    value = value_clone(value, ctx->allocator);
    context_declar(ctx, key, value);
    it = hash_map_node_get_next(it);
  }
  context_push_scope(ctx);
  bool variadic = false;
  size_t offset = 0;
  for (size_t idx = 0; idx < ast_get_length(arguments); idx++) {
    ast_node_t arg = ast_get_item(arguments, idx);
    ast_node_t identifier = ast_get_child(arg, "identifier");
    ast_node_t type = ast_get_child(arg, "type");
    ast_node_t mut = ast_get_child(arg, "mut");
    value_t vt = resolve_type(ctx, type);
    if (vt->type->kind == TYPE_KIND_ERROR) {
      err = vt;
      goto onerror;
    }
    type_t t = *(type_t *)vt->data;
    if (arg->type == NODE_TYPE_ARGUMENT_REST) {
      type_t arr_type = create_arr_type(ctx, t, argc - offset);
      value_t arr_value =
          context_create_comptime_value(ctx, arr_type, NULL, false, NULL);
      for (size_t idx = offset; idx < argc; idx++) {
        value_t val = argv[idx];
        value_t index = create_comptime_u64(ctx, idx - offset, false, NULL);
        err = value_set(arr_value, ctx, index, val);
        if (err->type->kind == TYPE_KIND_ERROR) {
          err = convert_comptime_error(ctx, node_get_location(arg), err);
          goto onerror;
        }
      }
      value_t arg = value_slice(arr_value, ctx, create_comptime_void(ctx),
                                create_comptime_void(ctx));
      arg = value_clone(arg, ctx->allocator);
      char *name = location_get(node_get_location(identifier), ctx->allocator);
      err = context_declar(ctx, name, arg);
      allocator_free(ctx->allocator, name);
      if (err->type->kind == TYPE_KIND_ERROR) {
        goto onerror;
      }
      t = arg->type;
      variadic = true;
    } else {
      value_t value = argv[offset++];
      value = value_safe_convert(value, ctx, t);
      if (value->type->kind == TYPE_KIND_ERROR) {
        err = value;
        err = convert_comptime_error(ctx, node_get_location(arg), err);
        goto onerror;
      }
      char *name = location_get(node_get_location(identifier), ctx->allocator);
      value = value_clone(value, ctx->allocator);
      err = context_declar(ctx, name, value);
      allocator_free(ctx->allocator, name);
      if (err->type->kind == TYPE_KIND_ERROR) {
        err = convert_comptime_error(ctx, node_get_location(arg), err);
        goto onerror;
      }
      ctype_t ctype = create_ctype(ctx->allocator, t, mut == NULL);
      array_push(args, ctype);
    }
  }
  value_t vt = resolve_type(ctx, type);
  if (vt->type->kind == TYPE_KIND_ERROR) {
    err = vt;
    err = convert_comptime_error(ctx, node_get_location(type), err);
    goto onerror;
  }
  type_t t = *(type_t *)vt->data;
  return_type = create_ctype(ctx->allocator, t, mut == NULL);
  type_t function_type = create_function_type(ctx, return_type, args, variadic);
  context_pop_scope(ctx);
  context_pop_scope(ctx);
  value_t func = create_function(ctx, function_type, node, declar->id);
  it = hash_map_get_first(declar->closure);
  while (it != hash_map_get_end(declar->closure)) {
    const char *key = hash_map_node_get_key(it);
    value_t value = hash_map_node_get_value(it);
    function_add_closure(func, ctx, key, value);
    it = hash_map_node_get_next(it);
  }
  allocator_free(ctx->allocator, return_type);
  allocator_free(ctx->allocator, args);
  return func;
onerror:
  allocator_free(ctx->allocator, args);
  allocator_free(ctx->allocator, return_type);
  if (err) {
    err = value_clone(err, ctx->allocator);
  }
  context_pop_scope(ctx);
  context_pop_scope(ctx);
  context_declar(ctx, NULL, err);
  return err;
}
value_t template_create_default_instance(value_t self, context_t ctx) {
  function_declar_t declar = *(function_declar_t *)self->data;
  ast_node_t node = declar->node;
  ast_node_t arguments = ast_get_child(node, "arguments");
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t mut = ast_get_child(node, "mut");
  value_t err = NULL;
  ctype_t return_type = NULL;
  array_t args = create_array(ctx->allocator, &(array_initialize_t){
                                                  .autofree = true,
                                              });
  context_push_scope(ctx);
  list_node_t it = hash_map_get_first(declar->closure);
  while (it != hash_map_get_end(declar->closure)) {
    const char *key = hash_map_node_get_key(it);
    value_t value = hash_map_node_get_value(it);
    value = value_clone(value, ctx->allocator);
    context_declar(ctx, key, value);
    it = hash_map_node_get_next(it);
  }
  context_push_scope(ctx);
  bool variadic = false;
  for (size_t idx = 0; idx < ast_get_length(arguments); idx++) {
    ast_node_t arg = ast_get_item(arguments, idx);
    ast_node_t identifier = ast_get_child(arg, "identifier");
    ast_node_t type = ast_get_child(arg, "type");
    ast_node_t mut = ast_get_child(arg, "mut");
    value_t vt = resolve_type(ctx, type);
    if (vt->type->kind == TYPE_KIND_ERROR) {
      err = vt;
      goto onerror;
    }
    type_t t = *(type_t *)vt->data;
    if (t->kind == TYPE_KIND_TYPE) {
      goto onerror;
    }
    char *name = location_get(node_get_location(identifier), ctx->allocator);
    err = context_create_value(ctx, t, mut == NULL, name);
    allocator_free(ctx->allocator, name);
    if (err->type->kind == TYPE_KIND_ERROR) {
      goto onerror;
    }
    ctype_t ctype = create_ctype(ctx->allocator, t, mut == NULL);
    array_push(args, ctype);
    if (arg->type == NODE_TYPE_ARGUMENT_REST) {
      variadic = true;
    }
  }
  value_t vt = resolve_type(ctx, type);
  if (vt->type->kind == TYPE_KIND_ERROR) {
    err = vt;
    goto onerror;
  }
  type_t t = *(type_t *)vt->data;
  return_type = create_ctype(ctx->allocator, t, mut == NULL);
  type_t function_type = create_function_type(ctx, return_type, args, variadic);
  context_pop_scope(ctx);
  context_pop_scope(ctx);
  value_t func = create_function(ctx, function_type, node, declar->id);
  it = hash_map_get_first(declar->closure);
  while (it != hash_map_get_end(declar->closure)) {
    const char *key = hash_map_node_get_key(it);
    value_t value = hash_map_node_get_value(it);
    function_add_closure(func, ctx, key, value);
    it = hash_map_node_get_next(it);
  }
  allocator_free(ctx->allocator, args);
  allocator_free(ctx->allocator, return_type);
  return func;
onerror:
  allocator_free(ctx->allocator, args);
  allocator_free(ctx->allocator, return_type);
  if (err) {
    err = value_clone(err, ctx->allocator);
  }
  context_pop_scope(ctx);
  context_pop_scope(ctx);
  context_declar(ctx, NULL, err);
  return err;
}