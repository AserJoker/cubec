#include "engine/value.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/location.h"
#include "core/string.h"
#include "engine/array.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/function.h"
#include "engine/module.h"
#include "engine/ptr.h"
#include "engine/scope.h"
#include "engine/slice.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/union.h"
#include "engine/unsigned.h"
#include "resolve/expression.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
struct _value_t {
  type_t type;
  bool mut;
  bool comptime;
  void *data;
};
static void value_dispose(value_t self, allocator_t allocator) {
  allocator_free(allocator, self->data);
}
value_t create_value(allocator_t allocator, type_t type, bool mut,
                     const void *data, bool comptime) {
  value_t self = allocator_alloc(allocator, sizeof(struct _value_t),
                                 (dispose_fn_t)value_dispose);
  self->type = type;
  self->mut = mut;
  size_t size = type_get_size(type);
  self->data = allocator_alloc(allocator, size, NULL);
  if (data) {
    memcpy(self->data, data, size);
  } else {
    memset(self->data, 0, size);
  }
  self->comptime = comptime;
  return self;
}
void value_set_mut(value_t self, bool mut) { self->mut = mut; }
value_t create_weak_value(allocator_t allocator, type_t type, bool mut,
                          void *data) {
  value_t self = allocator_alloc(allocator, sizeof(struct _value_t), NULL);
  self->type = type;
  self->mut = mut;
  size_t size = type_get_size(type);
  self->data = data;
  self->comptime = true;
  return self;
}
bool value_is_mut(value_t value) { return value->mut; }
bool value_is_comptime(value_t value) { return value->comptime; }
void *value_get_data(value_t value) { return value->data; }
type_t value_get_type(value_t value) { return value->type; }
value_t value_clone(value_t self, allocator_t allocator) {
  type_t type = value_get_type(self);
  const void *data = value_get_data(self);
  bool mut = value_is_mut(self);
  bool comptime = value_is_comptime(self);
  return create_value(allocator, type, mut, data, comptime);
}
value_t value_member_call(value_t self, struct _context_t *ctx,
                          const char *name, size_t argc, value_t argv[]) {
  type_t type = value_get_type(self);
  if (type_get_kind(type) == TYPE_KIND_TYPE) {
    type = *(type_t *)value_get_data(self);
    if (type_get_kind(type) == TYPE_KIND_STRUCT) {
      struct_attribute_t attr = struct_type_get_attribute(type, name);
      if (!attr) {
        return create_error(ctx, "no member %s in %s", name,
                            type_get_name(type));
      }
      if (!attr->pub) {
        type_t self = context_get_self(ctx);
        if (self != type) {
          return create_error(ctx, "identifier '%s' is not visible", name);
        }
      }
      return value_call(attr->value, ctx, argc, argv);
    } else if (type_get_kind(type) == TYPE_KIND_UNION) {
      union_attribute_t attr = union_type_get_attribute(type, name);
      if (!attr) {
        return create_error(ctx, "no member %s in %s", name,
                            type_get_name(type));
      }
      return value_call(attr->value, ctx, argc, argv);
    } else {
      return create_error(ctx, "no member %s in %s", name, type_get_name(type));
    }
  }
  value_t args[argc + 1];
  for (size_t idx = 0; idx < argc; idx++) {
    args[idx + 1] = argv[idx];
  }
  if (type_get_kind(type) != TYPE_KIND_PTR) {
    self = create_ptr_value(ctx, self);
  } else {
    type = ptr_type_get_type(type);
  }
  args[0] = self;
  value_t function = NULL;
  if (type_get_kind(type) == TYPE_KIND_STRUCT) {
    struct_attribute_t attr = struct_type_get_attribute(type, name);
    if (!attr) {
      return create_error(ctx, "no member %s in %s", name, type_get_name(type));
    }
    if (!attr->pub) {
      type_t self = context_get_self(ctx);
      if (self != type) {
        return create_error(ctx, "identifier '%s' is not visible", name);
      }
    }
    function = attr->value;
  } else if (type_get_kind(type) == TYPE_KIND_UNION) {
    union_attribute_t attr = union_type_get_attribute(type, name);
    if (!attr) {
      return create_error(ctx, "no member %s in %s", name, type_get_name(type));
    }
    function = attr->value;
  } else {
    return create_error(ctx, "no member %s in %s", name, type_get_name(type));
  }
  return value_call(function, ctx, argc + 1, args);
}
value_t value_convert(value_t self, struct _context_t *ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (strcmp(type_get_id(value_type), type_get_id(type)) == 0) {
    return context_create_weak_value(ctx, value_type, value_get_data(self),
                                     value_is_mut(self), NULL);
  }
  const type_operator_t *opt = type_get_operator(value_type);
  if (opt->convert) {
    return opt->convert(self, ctx, type);
  }
  return create_error(ctx, "cannot convert '%s' to '%s'",
                      type_get_name(value_type), type_get_name(type));
}
value_t value_safe_convert(value_t self, struct _context_t *ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (strcmp(type_get_id(value_type), type_get_id(type)) == 0) {
    return context_create_weak_value(ctx, value_type, value_get_data(self),
                                     value_is_mut(self), NULL);
  }
  const type_operator_t *opt = type_get_operator(value_type);
  if (opt->safe_convert) {
    return opt->safe_convert(self, ctx, type);
  }
  return create_error(ctx, "cannot convert '%s' to '%s'",
                      type_get_name(value_type), type_get_name(type));
}
value_t value_addr_of(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->addr_of) {
    return opt->addr_of(self, ctx);
  }
  return create_error(ctx, "unsupport operator for type '%s'",
                      type_get_name(type));
}
value_t value_deref(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->deref) {
    return opt->deref(self, ctx);
  }
  return create_error(ctx, "unsupport operator for type '%s'",
                      type_get_name(type));
}
value_t value_plus(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->plus) {
    return opt->plus(self, ctx);
  }
  return create_error(ctx, "unsupport operator for type '%s'",
                      type_get_name(type));
}
value_t value_neg(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->neg) {
    return opt->neg(self, ctx);
  }
  return create_error(ctx, "unsupport operator for type '%s'",
                      type_get_name(type));
}
value_t value_logical_not(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->logical_not) {
    return opt->logical_not(self, ctx);
  }
  return create_error(ctx, "unsupport operator for type '%s'",
                      type_get_name(type));
}
value_t value_bitwise_not(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->bitwise_not) {
    return opt->bitwise_not(self, ctx);
  }
  return create_error(ctx, "unsupport operator for type '%s'",
                      type_get_name(type));
}

value_t value_add(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->add) {
    return opt->add(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_sub(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->sub) {
    return opt->sub(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_mul(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->mul) {
    return opt->mul(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_div(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->div) {
    return opt->div(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_mod(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->mod) {
    return opt->mod(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_and(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->and_) {
    return opt->and_(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_or(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->or_) {
    return opt->or_(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_xor(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->xor_) {
    return opt->xor_(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_shl(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->shl) {
    return opt->shl(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_shr(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->shr) {
    return opt->shr(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_eq(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->eq) {
    return opt->eq(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_ne(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->ne) {
    return opt->ne(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_gt(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->gt) {
    return opt->gt(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_ge(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->ge) {
    return opt->ge(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_lt(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->lt) {
    return opt->lt(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_le(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->le) {
    return opt->le(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_logical_and(value_t self, struct _context_t *ctx,
                          value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->logical_and) {
    return opt->logical_and(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_logical_or(value_t self, struct _context_t *ctx,
                         value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->logical_or) {
    return opt->logical_or(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}

value_t value_get_field(value_t self, struct _context_t *ctx,
                        const char *name) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->get_field) {
    return opt->get_field(self, ctx, name);
  }
  return create_error(ctx, "type '%s' does not support field access",
                      type_get_name(type));
}
value_t value_set_field(value_t self, struct _context_t *ctx, const char *name,
                        value_t value) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->set_field) {
    return opt->set_field(self, ctx, name, value);
  }
  return create_error(ctx, "type '%s' does not support field access",
                      type_get_name(type));
}

value_t value_get(value_t self, struct _context_t *ctx, value_t key) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->get) {
    return opt->get(self, ctx, key);
  }
  return create_error(ctx, "type '%s' does not support field access",
                      type_get_name(type));
}
value_t value_set(value_t self, struct _context_t *ctx, value_t key,
                  value_t value) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->set) {
    return opt->set(self, ctx, key, value);
  }
  return create_error(ctx, "type '%s' does not support field access",
                      type_get_name(type));
}

value_t value_get_length(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->get_length) {
    return opt->get_length(self, ctx);
  }
  return create_error(ctx, "type '%s' does not support length access",
                      type_get_name(type));
}

static char *generator_func_id(context_t ctx, const char *base_name) {
  allocator_t allocator = context_get_allocator(ctx);
  module_t mod = context_get_module(ctx);
  for (size_t idx = 0;; idx++) {
    size_t len = snprintf(NULL, 0, "%s_%" PRIxPTR, base_name, idx);
    char buf[len];
    sprintf(buf, "%s_%" PRIxPTR, base_name, idx);
    if (!module_get_function(mod, buf)) {
      return create_cstring(allocator, buf);
    }
  }
}

static value_t infer_function(context_t ctx, value_t self, size_t argc,
                              value_t argv[]) {
  function_declar_t declar = *(function_declar_t *)value_get_data(self);
  type_t type = value_get_type(self);
  ast_node_t arguments = ast_get_child(declar->node, "arguments");
  ast_node_t type_node = ast_get_child(declar->node, "type");
  type_t current_global = context_set_global(ctx, declar->global);
  type_t current_self = context_set_self(ctx, declar->bind);
  scope_t current_scope = context_get_scope(ctx);
  allocator_t allocator = context_get_allocator(ctx);
  scope_t scope = create_scope(allocator, context_get_root_scope(ctx));
  value_t result = NULL;
  context_set_scope(ctx, scope);
  array_initialize_t closure_init = {
      .autofree = true,
  };
  array_t closure = create_array(allocator, &closure_init);
  for (size_t idx = 0; idx < array_get_size(declar->closure); idx++) {
    closure_item_t item = array_get(declar->closure, idx);
    value_t value = value_clone(item->value, allocator);
    value_t err = context_declar(ctx, item->name, value);
    if (value_is_error(err)) {
      return err;
    }
    closure_item_t new_item = create_closure_item(allocator, item->name, value);
    array_push(closure, new_item);
  }
  context_push_scope(ctx);
  array_initialize_t init = {
      .autofree = true,
  };
  array_t args = create_array(allocator, &init);
  bool variadic = false;
  for (size_t idx = 0; idx < ast_get_length(arguments); idx++) {
    ast_node_t arg_node = ast_get_item(arguments, idx);
    ast_node_t type = ast_get_child(arg_node, "type");
    ast_node_t mut = ast_get_child(arg_node, "mut");
    ast_node_t identifier = ast_get_child(arg_node, "identifier");
    argument_t arg =
        allocator_alloc(allocator, sizeof(struct _argument_t), NULL);
    array_push(args, arg);
    arg->mut = mut == NULL;
    arg->type = NULL;
    if (arg_node->type == NODE_TYPE_FUNCTION_ARGUMENT_REST) {
      if (type == NULL) {
        break;
      }
      variadic = true;
      type_t base_type = NULL;
      if (location_is(type->loc, "infer")) {
        base_type = NULL;
        if (idx >= argc) {
          base_type = context_load_type(ctx, "null");
        } else {
          base_type = value_get_type(argv[idx]);
        }
        arg->type = create_slice_type(ctx, base_type);
        if (type_get_kind(arg->type) == TYPE_KIND_PTR ||
            type_get_kind(arg->type) == TYPE_KIND_PARRAY ||
            type_get_kind(arg->type) == TYPE_KIND_OPAQUE ||
            type_get_kind(arg->type) == TYPE_KIND_SLICE) {
          arg->mut = value_is_mut(argv[idx]);
        }
      } else {
        value_t vtype = resolve_expression(ctx, type);
        if (value_is_error(vtype)) {
          result = vtype;
          goto onfinish;
        }
        arg->type = *(type_t *)value_get_type(vtype);
        if (type_get_kind(arg->type) != TYPE_KIND_SLICE) {
          result = create_comptime_error(ctx, type,
                                         "rest argument type is not slice");
          goto onfinish;
        }
        base_type = slice_type_get_type(arg->type);
      }
      size_t len = argc - idx;
      type_t arr_type = create_array_type(ctx, base_type, len);
      value_t arr = NULL;
      if (context_is_comptime(ctx)) {
        uint8_t data[type_get_size(arr_type)];
        memset(data, 0, type_get_size(arr_type));
        arr = context_create_value(ctx, arr_type, data, true, true, NULL);
      } else {
        arr = context_create_value(ctx, arr_type, NULL, true, false, NULL);
      }
      for (size_t i = 0; idx + i < argc; i++) {
        value_t val = argv[idx + i];
        value_t key = create_comptime_u64(ctx, i, false, NULL);
        value_t err = value_set(arr, ctx, key, val);
        if (value_is_error(err)) {
          result = err;
          goto onfinish;
        }
      }
      value_t err = NULL;
      char *name = location_get(identifier->loc, allocator);
      if (strcmp(name, "_") != 0) {
        if (context_is_comptime(ctx)) {
          void *data = value_get_data(arr);
          value_t slice =
              create_comptime_slice(ctx, arg->type, data, 0, len, arg->mut);
          slice = value_clone(slice, allocator);
          err = context_declar(ctx, name, slice);
        } else {
          err =
              context_create_value(ctx, arg->type, NULL, arg->mut, false, name);
        }
      }
      allocator_free(allocator, name);
      if (value_is_error(err)) {
        result = err;
        goto onfinish;
      }
    } else {
      if (idx >= argc) {
        result = create_error(ctx, "missing argument %" PRIuPTR, idx);
        goto onfinish;
      }
      if (location_is(type->loc, "infer")) {
        arg->type = value_get_type(argv[idx]);
        if (type_get_kind(arg->type) == TYPE_KIND_PTR ||
            type_get_kind(arg->type) == TYPE_KIND_PARRAY ||
            type_get_kind(arg->type) == TYPE_KIND_OPAQUE ||
            type_get_kind(arg->type) == TYPE_KIND_SLICE) {
          arg->mut = value_is_mut(argv[idx]);
        }
      } else {
        value_t vtype = resolve_expression(ctx, type);
        if (value_is_error(vtype)) {
          result = vtype;
          goto onfinish;
        }
        arg->type = *(type_t *)value_get_data(vtype);
      }
      if (type_get_kind(arg->type) == TYPE_KIND_VOID) {
        result = create_comptime_error(ctx, arg_node,
                                       "cannot declar value with void");
        goto onfinish;
      }
      if (type_get_kind(arg->type) == TYPE_KIND_TYPE) {
        if (!context_is_comptime(ctx)) {
          result = create_comptime_error(
              ctx, arg_node, "type value on used in comptime context");
          goto onfinish;
        }
      }
      value_t value = argv[idx];
      if (context_is_comptime(ctx)) {
        if (!value_is_comptime(value)) {
          result = create_error(ctx, "value is not comptime");
          goto onfinish;
        }
      }
      value = value_safe_convert(value, ctx, arg->type);
      if (value_is_error(value)) {
        result = value;
        goto onfinish;
      }
      value_t err = NULL;
      char *name = location_get(identifier->loc, allocator);
      if (strcmp(name, "_") != 0) {
        value = value_clone(value, allocator);
        err = context_declar(ctx, name, value);
      }
      allocator_free(allocator, name);
      if (value_is_error(err)) {
        result = err;
        goto onfinish;
      }
    }
  }
  value_t vtype = resolve_expression(ctx, type_node);
  if (value_is_error(vtype)) {
    result = vtype;
    goto onfinish;
  }
  type_t return_type = *(type_t *)value_get_data(vtype);
  type_t function_type = create_function_type(ctx, return_type, args, variadic);
  args = NULL;
  char *id = NULL;
  if (type_get_kind(type) == TYPE_KIND_TEMPLATE_FUNCTION) {
    array_t args = function_type_get_arguments(function_type);
    string_t sid = create_string(allocator, NULL);
    string_concat(sid, allocator, declar->id);
    string_concat(sid, allocator, "R");
    string_concat(sid, allocator, type_get_id(return_type));
    for (size_t idx = 0; idx < array_get_size(args); idx++) {
      string_concat(sid, allocator, "A");
      argument_t arg = array_get(args, idx);
      string_concat(sid, allocator, type_get_id(arg->type));
    }
    const char *cid = string_get(sid);
    id = create_cstring(allocator, cid);
    allocator_free(allocator, sid);
  } else {
    id = generator_func_id(ctx, declar->id);
  }
  module_t mod = context_get_module(ctx);
  result = module_get_function(mod, id);
  if (result) {
    allocator_free(allocator, id);
    goto onfinish;
  }
  declar = create_function_declar(allocator, FUNC_TYPE_AST, declar->global,
                                  declar->bind, id, declar->node, closure);
  closure = NULL;
  allocator_free(allocator, id);
  context_store_function_declar(ctx, declar);
  result = context_create_value(ctx, function_type, &declar, false, true, NULL);
onfinish:
  allocator_free(allocator, closure);
  allocator_free(allocator, args);
  result = value_clone(result, allocator);
  scope_store(current_scope, allocator, NULL, result);
  context_set_scope(ctx, current_scope);
  allocator_free(allocator, scope);
  return result;
}

value_t value_call(value_t self, struct _context_t *ctx, size_t argc,
                   value_t argv[]) {
  type_t type = value_get_type(self);
  type_t raw_type = type;
  if (value_is_comptime(self)) {
    if (type_get_kind(type) == TYPE_KIND_COMPTIME_FUNCTION ||
        type_get_kind(type) == TYPE_KIND_TEMPLATE_FUNCTION) {
      bool is_comptime = context_set_comptime(ctx, true);
      self = infer_function(ctx, self, argc, argv);
      context_set_comptime(ctx, is_comptime);
      if (value_is_error(self)) {
        return self;
      }
      type = value_get_type(self);
    }
  } else {
    if (type_get_kind(type) == TYPE_KIND_COMPTIME_FUNCTION) {
      bool is_comptime = context_set_comptime(ctx, true);
      self = infer_function(ctx, self, argc, argv);
      context_set_comptime(ctx, is_comptime);
      if (value_is_error(self)) {
        return self;
      }
      type = value_get_type(self);
    } else if (type_get_kind(type) == TYPE_KIND_TEMPLATE_FUNCTION) {
      self = infer_function(ctx, self, argc, argv);
      if (value_is_error(self)) {
        return self;
      }
      type = value_get_type(self);
    }
  }
  if (type_get_kind(raw_type) == TYPE_KIND_TEMPLATE_FUNCTION) {
    function_declar_t declar = *(function_declar_t *)value_get_data(self);
    module_t current = context_get_module(ctx);
    if (!module_get_function(current, declar->id)) {
      allocator_t allocator = context_get_allocator(ctx);
      self = value_clone(self, allocator);
      module_add_function(current, self);
    }
  }
  const type_operator_t *opt = type_get_operator(type);
  if (opt->call) {
    bool is_comptime = context_is_comptime(ctx);
    if (type_get_kind(raw_type) == TYPE_KIND_COMPTIME_FUNCTION) {
      context_set_comptime(ctx, true);
    }
    value_t res = opt->call(self, ctx, argc, argv);
    context_set_comptime(ctx, is_comptime);
    return res;
  }
  return create_error(ctx, "type '%s' is not callable", type_get_name(type));
}

value_t value_assigment(value_t self, struct _context_t *ctx, value_t value) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(value);
  if (opt->assigment) {
    return opt->assigment(self, ctx, value);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_slice(value_t self, struct _context_t *ctx, value_t start,
                    value_t end) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->slice) {
    return opt->slice(self, ctx, start, end);
  }
  return create_error(ctx, "invalid operands to %s", type_get_name(type));
}
value_t value_default_assigment(value_t self, struct _context_t *ctx,
                                value_t value) {
  if (!value_is_mut(self)) {
    return create_error(ctx, "value is not mutable");
  }
  type_t dst_type = value_get_type(self);
  value = value_safe_convert(value, ctx, dst_type);
  if (value_is_error(value)) {
    return value;
  }
  if (value_is_comptime(value)) {
    const void *data = value_get_data(value);
    if (value_is_comptime(self)) {
      memcpy(self->data, data, type_get_size(dst_type));
    }
  } else {
    if (value_is_comptime(self)) {
      return create_error(ctx, "value is not comptime");
    }
  }
  return self;
}
value_t value_default_address_of(value_t self, struct _context_t *ctx) {
  return create_ptr_value(ctx, self);
}
bool value_is_error(value_t value) {
  type_t type = value_get_type(value);
  return type_get_kind(type) == TYPE_KIND_ERROR ||
         type_get_kind(type) == TYPE_KIND_FORMAT_ERROR;
}
bool value_is_interrupt(value_t value) {
  type_t type = value_get_type(value);
  return type_get_kind(type) == TYPE_KIND_INTERRUPT;
}