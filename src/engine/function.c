#include "engine/function.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/hash.h"
#include "core/hash_map.h"
#include "core/list.h"
#include "core/location.h"
#include "core/position.h"
#include "core/string.h"
#include "engine/arr.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/module.h"
#include "engine/scope.h"
#include "engine/slice.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include "engine/void.h"
#include "resolve/statement_block.h"
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
                            sizeof(function_declar_t), &opt, NULL, true);
  context_store_type(ctx, type);
}
static void function_declar_dispose(function_declar_t self,
                                    allocator_t allocator) {
  if (self->kind == FUNCTION_KIND_NORMAL ||
      self->kind == FUNCTION_KIND_COMPTIME ||
      self->kind == FUNCTION_KIND_TEMPLATE) {
    allocator_free(allocator, self->node);
  }
  allocator_free(allocator, self->id);
  allocator_free(allocator, self->closure);
  allocator_free(allocator, self->template_id);
}

function_declar_t create_function_declar(allocator_t allocator,
                                         function_kind_t kind,
                                         const char *template_id,
                                         const char *id, type_t self,
                                         module_t mod) {
  function_declar_t declar =
      allocator_alloc(allocator, sizeof(struct _function_declar_t),
                      (dispose_fn_t)function_declar_dispose);
  declar->self = self;
  declar->mod = mod;
  declar->id = create_cstring(allocator, id);
  declar->data = NULL;
  declar->kind = kind;
  declar->template_id = create_cstring(allocator, template_id);
  declar->closure =
      create_hash_map(allocator, &(hash_map_initialize_t){
                                     .autofree_key = true,
                                     .autofree_value = true,
                                     .hash = (hash_fn_t)cstring_sdb,
                                     .compare = (compare_fn_t)strcmp,
                                 });
  return declar;
}

static void function_meta_dispsoe(function_meta_t self, allocator_t allocator) {
  allocator_free(allocator, self->type);
  allocator_free(allocator, self->args);
  allocator_free(allocator, self->closure);
}
static function_meta_t create_function_meta(allocator_t allocator, ctype_t type,
                                            array_t args, bool variadic,
                                            hash_map_t closure) {
  function_meta_t self =
      allocator_alloc(allocator, sizeof(struct _function_meta_t),
                      (dispose_fn_t)function_meta_dispsoe);
  self->type = create_ctype(allocator, type->type, type->mut);
  self->variadic = variadic;
  self->args = create_array(allocator, &(array_initialize_t){.autofree = true});
  self->closure =
      create_hash_map(allocator, &(hash_map_initialize_t){
                                     .autofree_key = true,
                                     .autofree_value = false,
                                     .compare = (compare_fn_t)strcmp,
                                     .hash = (hash_fn_t)cstring_sdb,
                                 });
  for (size_t idx = 0; idx < array_get_size(args); idx++) {
    ctype_t arg = array_get(args, idx);
    array_push(self->args, create_ctype(allocator, arg->type, arg->mut));
  }
  if (closure) {
    list_node_t it = hash_map_get_first(closure);
    while (it != hash_map_get_end(closure)) {
      const char *key = hash_map_node_get_key(it);
      type_t type = hash_map_node_get_value(it);
      hash_map_set(self->closure, create_cstring(allocator, key), type, NULL,
                   NULL);
      it = hash_map_node_get_next(it);
    }
  }
  return self;
}

static value_t function_call(value_t self, context_t ctx, size_t argc,
                             value_t *argv) {
  function_declar_t declar =
      self->data ? *(function_declar_t *)self->data : NULL;
  type_t func_type = self->type;
  function_meta_t meta = func_type->meta;
  size_t arg_count = array_get_size(meta->args);
  value_t args[arg_count];
  bool is_comptime =
      ctx->comptime || (declar && declar->kind == FUNCTION_KIND_COMPTIME);
  if (ctx->type == CONTEXT_TYPE_STRUCT) {
    if (!declar || declar->kind != FUNCTION_KIND_COMPTIME) {
      return create_error(ctx, "function is not comptime");
    }
  }
  if (meta->variadic) {
    if (argc < arg_count - 1) {
      return create_error(
          ctx, "function requires %" PRIuPTR " arguments, receive %" PRIuPTR,
          arg_count - 1, argv);
    }
  } else {
    if (argc < arg_count) {
      return create_error(
          ctx, "function requires %" PRIuPTR " arguments, receive %" PRIuPTR,
          arg_count, argv);
    }
  }
  for (size_t idx = 0; idx < arg_count; idx++) {
    ctype_t ctype = array_get(meta->args, idx);
    if (meta->variadic && idx == arg_count - 1) {
      type_t arr_type = create_arr_type(ctx, ctype->type, argc - arg_count);
      value_t arr = NULL;
      if (is_comptime) {
        arr = context_create_comptime_value(ctx, arr_type, NULL, true, NULL);
      } else {
        arr = context_create_value(ctx, arr_type, true, NULL);
      }
      for (size_t i = idx; i < arg_count; i++) {
        value_t key = create_comptime_u64(ctx, i, false, NULL);
        value_t err = value_set(arr, ctx, key, argv[idx + i]);
        if (err->type->kind == TYPE_KIND_ERROR) {
          return err;
        }
      }
      value_t value = value_slice(arr, ctx, create_comptime_void(ctx),
                                  create_comptime_void(ctx));
      if (value->type->kind == TYPE_KIND_ERROR) {
        return value;
      }
      value->mut = ctype->mut;
      args[idx] = value;
    } else {
      value_t value = value_safe_convert(argv[idx], ctx, ctype->type);
      if (value->type->kind == TYPE_KIND_ERROR) {
        return value;
      }
      if (ctype->type->kind == TYPE_KIND_PTR) {
        if (ctype->mut && !value->mut) {
          return create_error(ctx,
                              "cannot assigment const ptr to non-const ptr");
        }
      }
      value->mut = ctype->mut;
      args[idx] = value;
    }
  }
  if (declar) {
    if (declar->kind == FUNCTION_KIND_NATIVE) {
      return declar->handle(ctx, argc, argv);
    }
    if (declar->kind == FUNCTION_KIND_EXTERN) {
      if (ctx->comptime) {
        return create_error(ctx,
                            "cannot call extern function in comptime context");
      } else {
        return context_create_value(ctx, meta->type->type, meta->type->mut,
                                    NULL);
      }
    }
  }
  if (is_comptime) {
    ast_node_t body = ast_get_child(declar->node, "body");
    bool current_comptime = ctx->comptime;
    ctx->comptime = true;
    value_t current_function = ctx->function;
    ctx->function = self;
    type_t current_self = ctx->self;
    ctx->self = declar->self;
    type_t current_global = ctx->global;
    ctx->global = *(type_t *)declar->mod->value->data;
    module_t current_module = ctx->mod;
    ctx->mod = declar->mod;
    context_type_t current_type = ctx->type;
    ctx->type = CONTEXT_TYPE_FUNCTION;
    scope_t current_scope = ctx->current;
    scope_t scope = create_scope(ctx->allocator, ctx->root);
    ctx->current = scope;
    value_t result = NULL;
    ast_node_t arguments = ast_get_child(declar->node, "arguments");
    size_t offset = 0;
    for (size_t idx = 0; idx < arg_count; idx++) {
      ast_node_t arg = ast_get_item(arguments, idx);
      ast_node_t identifier = ast_get_child(arg, "identifier");
      ast_node_t type = ast_get_child(arg, "type");
      ast_node_t mut = ast_get_child(arg, "mut");
      value_t vt = resolve_type(ctx, type);
      type_t t = *(type_t *)vt->type;
      char *name = location_get(node_get_location(identifier), ctx->allocator);
      value_t val = args[offset++];
      val = value_clone(val, ctx->allocator);
      value_t err = context_declar(ctx, name, val);
      if (err->type->kind == TYPE_KIND_ERROR) {
        result = err;
        allocator_free(ctx->allocator, name);
        break;
      }
      allocator_free(ctx->allocator, name);
    }
    if (!result) {
      context_push_scope(ctx);
      result = resolve_statement_block(ctx, body);
    }
    result = value_clone(result, ctx->allocator);
    scope_store(current_scope, NULL, result);
    allocator_free(ctx->allocator, scope);
    ctx->current = current_scope;
    ctx->comptime = current_comptime;
    ctx->type = current_type;
    ctx->mod = current_module;
    ctx->self = current_self;
    ctx->global = current_global;
    ctx->function = current_function;
    return result;
  } else {
    return context_create_value(ctx, meta->type->type, meta->type->mut, NULL);
  }
}

static bool function_type_is_equal(type_t self, type_t another) {
  if (another->kind != TYPE_KIND_FUNCTION) {
    return false;
  }
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
                            bool variadic, hash_map_t closure) {
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
  if (closure) {
    list_node_t it = hash_map_get_first(closure);
    while (it != hash_map_get_end(closure)) {
      char *key = hash_map_node_get_key(it);
      type_t type = hash_map_node_get_value(it);
      len += strlen(key);
      len += strlen(type->id);
      len += 2;
      it = hash_map_node_get_next(it);
    }
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
  if (closure) {
    list_node_t it = hash_map_get_first(closure);
    while (it != hash_map_get_end(closure)) {
      char *key = hash_map_node_get_key(it);
      type_t type = hash_map_node_get_value(it);
      id[offset++] = 'B';
      strcpy(&id[offset], key);
      offset += strlen(key);
      id[offset++] = 'T';
      strcpy(&id[offset], type->id);
      offset += strlen(type->id);
      it = hash_map_node_get_next(it);
    }
  }
  id[offset] = 0;
  type_t func_type = context_load_type(ctx, id);
  if (!func_type) {
    bool is_comptime = false;
    size_t len = strlen("func(");
    if (closure && hash_map_get_size(closure)) {
      len++;
      list_node_t it = hash_map_get_first(closure);
      while (it != hash_map_get_end(closure)) {
        if (it != hash_map_get_first(closure)) {
          len += 2;
        }
        char *key = hash_map_node_get_key(it);
        type_t type = hash_map_node_get_value(it);
        len += strlen(key);
        len++;
        len += strlen(type->name);
        it = hash_map_node_get_next(it);
      }
      len++;
    }
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
    strcpy(&name[offset], "func");
    offset += strlen("func");
    if (closure && hash_map_get_size(closure)) {
      name[offset++] = '[';
      list_node_t it = hash_map_get_first(closure);
      while (it != hash_map_get_end(closure)) {
        if (it != hash_map_get_first(closure)) {
          name[offset++] = ',';
          name[offset++] = ' ';
        }
        char *key = hash_map_node_get_key(it);
        type_t type = hash_map_node_get_value(it);
        strcpy(&name[offset], key);
        offset += strlen(key);
        name[offset++] = ':';
        strcpy(&name[offset], type->name);
        offset += strlen(type->name);
        it = hash_map_node_get_next(it);
      }
      name[offset++] = ']';
    }
    name[offset++] = '(';
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
        is_comptime |= arg->type->comptime;
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
      is_comptime |= type->type->comptime;
    }
    name[offset] = 0;
    struct _type_operator_t opt = {
        .call = function_call,
        .type_equal = function_type_is_equal,
    };
    function_meta_t meta =
        create_function_meta(ctx->allocator, type, argv, variadic, closure);
    func_type =
        create_type(ctx->allocator, TYPE_KIND_FUNCTION, name, id,
                    sizeof(void *), sizeof(void *), &opt, meta, is_comptime);
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
  ast_node_t kind = ast_get_child(node, "kind");
  bool is_comptime = kind && node_location_is(kind, "comptime");
  function_declar_t declar = create_function_declar(
      ctx->allocator,
      is_comptime ? FUNCTION_KIND_COMPTIME : FUNCTION_KIND_NORMAL, base_id, id,
      ctx->self, ctx->mod);
  declar->node = clone_ast_node(ctx->allocator, node);
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
  } else {
    len += 16;
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
  function_declar_t declar = create_function_declar(
      ctx->allocator, FUNCTION_KIND_TEMPLATE, id, id, ctx->self, ctx->mod);
  declar->node = clone_ast_node(ctx->allocator, node);
  array_push(ctx->functions, declar);
  allocator_free(ctx->allocator, id);
  return context_create_comptime_value(ctx, type, &declar, false, NULL);
}

static value_t resolve_function_declaration(context_t ctx, value_t function) {
  function_declar_t declar = *(function_declar_t *)function->data;
  type_t func_type = function->type;
  function_meta_t meta = func_type->meta;
  size_t arg_count = array_get_size(meta->args);
  ast_node_t body = ast_get_child(declar->node, "body");
  bool current_comptime = ctx->comptime;
  ctx->comptime = false;
  value_t current_function = ctx->function;
  ctx->function = function;
  type_t current_self = ctx->self;
  ctx->self = declar->self;
  type_t current_global = ctx->global;
  ctx->global = *(type_t *)declar->mod->value->data;
  module_t current_module = ctx->mod;
  ctx->mod = declar->mod;
  context_type_t current_type = ctx->type;
  ctx->type = CONTEXT_TYPE_FUNCTION;
  scope_t current_scope = ctx->current;
  scope_t scope = create_scope(ctx->allocator, ctx->root);
  ctx->current = scope;
  declar->node->scope = scope;
  value_t result = NULL;
  ast_node_t arguments = ast_get_child(declar->node, "arguments");
  for (size_t idx = 0; idx < arg_count; idx++) {
    ast_node_t arg = ast_get_item(arguments, idx);
    ast_node_t identifier = ast_get_child(arg, "identifier");
    ast_node_t mut = ast_get_child(arg, "mut");
    ctype_t ctype = array_get(meta->args, idx);
    type_t type = ctype->type;
    if (type) {
      if (arg->type == NODE_TYPE_ARGUMENT_REST) {
        type = create_slice_type(ctx, type);
      }
      char *name = location_get(node_get_location(identifier), ctx->allocator);
      value_t err = context_create_value(ctx, type, ctype->mut, name);
      if (err->type->kind == TYPE_KIND_ERROR) {
        result = err;
        allocator_free(ctx->allocator, name);
        break;
      }
      allocator_free(ctx->allocator, name);
    }
  }
  if (!result) {
    result = resolve_statement_block(ctx, body);
  }
  result = value_clone(result, ctx->allocator);
  scope_store(current_scope, NULL, result);
  ctx->current = current_scope;
  ctx->comptime = current_comptime;
  ctx->type = current_type;
  ctx->mod = current_module;
  ctx->self = current_self;
  ctx->global = current_global;
  ctx->function = current_function;
  return result;
}

value_t template_create_instance(value_t self, context_t ctx, size_t argc,
                                 value_t *argv) {
  function_declar_t declar = *(function_declar_t *)self->data;
  ast_node_t node = clone_ast_node(ctx->allocator, declar->node);
  ast_node_t arguments = ast_get_child(node, "arguments");
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t mut = ast_get_child(node, "mut");
  value_t err = NULL;
  ctype_t return_type = NULL;
  array_t args = create_array(ctx->allocator, &(array_initialize_t){
                                                  .autofree = true,
                                              });
  scope_t current_scope = ctx->current;
  scope_t scope = create_scope(ctx->allocator, ctx->root);
  ctx->current = scope;
  value_t current_function = ctx->function;
  ctx->function = self;
  bool variadic = false;
  size_t offset = 0;
  for (size_t idx = 0; idx < ast_get_length(arguments); idx++) {
    ast_node_t arg = ast_get_item(arguments, idx);
    ast_node_t identifier = ast_get_child(arg, "identifier");
    ast_node_t type = ast_get_child(arg, "type");
    ast_node_t mut = ast_get_child(arg, "mut");
    type_t t = NULL;
    if (type) {
      if (node_location_is(type, "infer")) {
        t = argv[idx]->type;
      } else {
        value_t vt = resolve_type(ctx, type);
        if (vt->type->kind == TYPE_KIND_ERROR) {
          err = vt;
          goto onerror;
        }
        t = *(type_t *)vt->data;
      }
    }
    if (arg->type == NODE_TYPE_ARGUMENT_REST) {
      if (!t) {
        break;
      }
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
  hash_map_t closure =
      create_hash_map(ctx->allocator, &(hash_map_initialize_t){
                                          .autofree_key = false,
                                          .autofree_value = false,
                                          .compare = (compare_fn_t)strcmp,
                                          .hash = (hash_fn_t)cstring_sdb,
                                      });
  list_node_t it = hash_map_get_first(declar->closure);
  while (it != hash_map_get_end(declar->closure)) {
    const char *key = hash_map_node_get_key(it);
    value_t value = hash_map_node_get_value(it);
    if (!value->comptime) {
      hash_map_set(closure, (void *)key, value->type, NULL, NULL);
    }
    it = hash_map_node_get_next(it);
  }
  type_t function_type =
      create_function_type(ctx, return_type, args, variadic, closure);
  allocator_free(ctx->allocator, closure);
  allocator_free(ctx->allocator, return_type);
  allocator_free(ctx->allocator, args);
  ctx->current = current_scope;
  allocator_free(ctx->allocator, scope);
  ctx->function = current_function;
  value_t func = NULL;
  it = hash_map_get_first(ctx->mod->functions);
  while (it != hash_map_get_end(ctx->mod->functions)) {
    value_t value = hash_map_node_get_value(it);
    function_declar_t dec = *(function_declar_t *)value->data;
    if (strcmp(dec->template_id, declar->id) == 0) {
      if (type_is_equal(function_type, value->type)) {
        func = value_clone(value, ctx->allocator);
        context_declar(ctx, NULL, func);
        break;
      }
    }
    it = hash_map_node_get_next(it);
  }
  if (!func) {
    func = create_function(ctx, function_type, node, declar->id);
    allocator_free(ctx->allocator, node);
    it = hash_map_get_first(declar->closure);
    while (it != hash_map_get_end(declar->closure)) {
      const char *key = hash_map_node_get_key(it);
      value_t value = hash_map_node_get_value(it);
      function_add_closure(func, ctx, key, value);
      it = hash_map_node_get_next(it);
    }
    declar = *(function_declar_t *)func->data;
    if (declar->kind == FUNCTION_KIND_NORMAL) {
      value_t err = resolve_function_declaration(ctx, func);
      if (err->type->kind == TYPE_KIND_ERROR) {
        goto onerror;
      }
    }
  }
  return func;
onerror:
  allocator_free(ctx->allocator, node);
  allocator_free(ctx->allocator, args);
  allocator_free(ctx->allocator, return_type);
  if (err) {
    err = value_clone(err, ctx->allocator);
  }
  ctx->current = current_scope;
  allocator_free(ctx->allocator, scope);
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
  scope_t current_scope = ctx->current;
  scope_t scope = create_scope(ctx->allocator, ctx->root);
  ctx->current = scope;
  value_t current_function = ctx->function;
  ctx->function = self;
  bool variadic = false;
  for (size_t idx = 0; idx < ast_get_length(arguments); idx++) {
    ast_node_t arg = ast_get_item(arguments, idx);
    ast_node_t identifier = ast_get_child(arg, "identifier");
    ast_node_t type = ast_get_child(arg, "type");
    ast_node_t mut = ast_get_child(arg, "mut");
    type_t t = NULL;
    if (type) {
      if (node_location_is(type, "infer")) {
        err = NULL;
        goto onerror;
      }
      value_t vt = resolve_type(ctx, type);
      if (vt->type->kind == TYPE_KIND_ERROR) {
        err = vt;
        goto onerror;
      }
      t = *(type_t *)vt->data;
      if (t->kind == TYPE_KIND_TYPE) {
        err = NULL;
        goto onerror;
      }
      char *name = location_get(node_get_location(identifier), ctx->allocator);
      err = context_create_value(ctx, t, mut == NULL, name);
      allocator_free(ctx->allocator, name);
      if (err->type->kind == TYPE_KIND_ERROR) {
        goto onerror;
      }
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
  hash_map_t closure =
      create_hash_map(ctx->allocator, &(hash_map_initialize_t){
                                          .autofree_key = false,
                                          .autofree_value = false,
                                          .compare = (compare_fn_t)strcmp,
                                          .hash = (hash_fn_t)cstring_sdb,
                                      });
  list_node_t it = hash_map_get_first(declar->closure);
  while (it != hash_map_get_end(declar->closure)) {
    const char *key = hash_map_node_get_key(it);
    value_t value = hash_map_node_get_value(it);
    if (!value->comptime) {
      hash_map_set(closure, (void *)key, value->type, NULL, NULL);
    }
    it = hash_map_node_get_next(it);
  }
  type_t function_type =
      create_function_type(ctx, return_type, args, variadic, closure);
  allocator_free(ctx->allocator, closure);
  allocator_free(ctx->allocator, args);
  allocator_free(ctx->allocator, return_type);
  ctx->current = current_scope;
  ctx->function = current_function;
  value_t func = create_function(ctx, function_type, node, declar->id);
  it = hash_map_get_first(declar->closure);
  while (it != hash_map_get_end(declar->closure)) {
    const char *key = hash_map_node_get_key(it);
    value_t value = hash_map_node_get_value(it);
    function_add_closure(func, ctx, key, value);
    it = hash_map_node_get_next(it);
  }
  declar = *(function_declar_t *)func->data;
  if (declar->kind == FUNCTION_KIND_NORMAL) {
    value_t err = resolve_function_declaration(ctx, func);
    if (err->type->kind == TYPE_KIND_ERROR) {
      return err;
    }
  }
  return func;
onerror:
  allocator_free(ctx->allocator, args);
  allocator_free(ctx->allocator, return_type);
  if (err) {
    err = value_clone(err, ctx->allocator);
  }
  ctx->current = current_scope;
  allocator_free(ctx->allocator, scope);
  context_declar(ctx, NULL, err);
  return err;
}