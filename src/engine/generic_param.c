#include "engine/generic_param.h"
#include "engine/enum_type.h"
#include "core/string.h"

/* ---- generic_param_t class ---- */

static void _generic_param_init(void *self, allocator_t allocator, void *arg) {
  generic_param_t gp = (generic_param_t)self;
  generic_param_init_t *init = (generic_param_init_t *)arg;

  gp->name = init->name ? cstring_clone(allocator, init->name) : NULL;
  gp->type = init->type; /* borrowed: types managed by vm->types */
  gp->is_rest = init->is_rest;

  vec_init_t vi = {.auto_dispose = false};
  gp->extends = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  if (init->extends) {
    size_t n = vec_get_size(init->extends);
    for (size_t i = 0; i < n; i++) {
      type_t t = (type_t)vec_get(init->extends, i);
      vec_push(gp->extends, t);
    }
  }
}

static void _generic_param_dispose(void *self, allocator_t allocator) {
  generic_param_t gp = (generic_param_t)self;
  if (gp->name) {
    void *p = gp->name;
    allocator_free(allocator, &p);
    gp->name = NULL;
  }
  /* type is borrowed from vm->types — do not free */
  gp->type = NULL;
  allocator_free(allocator, &gp->extends);
}

static void _generic_param_clone(void *self, allocator_t allocator, void *another) {
  generic_param_t dst = (generic_param_t)self;
  generic_param_t src = (generic_param_t)another;

  dst->name = src->name ? cstring_clone(allocator, src->name) : NULL;
  dst->type = src->type; /* borrowed: types managed by vm->types */
  dst->is_rest = src->is_rest;

  vec_init_t vi = {.auto_dispose = false};
  dst->extends = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  size_t n = vec_get_size(src->extends);
  for (size_t i = 0; i < n; i++) {
    type_t t = (type_t)vec_get(src->extends, i);
    vec_push(dst->extends, t);
  }
}

class_t g_generic_param_class = {
    .size    = sizeof(struct _generic_param_t),
    .name    = "cubec.engine.generic_param",
    .init    = (class_init_fn_t)_generic_param_init,
    .dispose = (class_dispose_fn_t)_generic_param_dispose,
    .clone   = (class_clone_fn_t)_generic_param_clone,
    .move    = NULL,
};

/* ---- Public API ---- */

generic_param_t generic_param_create(allocator_t allocator, const char *name,
                                     type_t type, vec_t extends, bool is_rest) {
  generic_param_init_t init = {
      .name    = name,
      .type    = type,
      .extends = extends,
      .is_rest = is_rest,
  };
  return (generic_param_t)allocator_create(allocator, &g_generic_param_class, &init);
}

const char *generic_param_get_name(generic_param_t self) { return self->name; }
type_t      generic_param_get_type(generic_param_t self) { return self->type; }
vec_t       generic_param_get_extends(generic_param_t self) { return self->extends; }
bool        generic_param_is_rest(generic_param_t self) { return self->is_rest; }

bool generic_param_is_value_type_allowed(type_t t) {
  type_kind_t kind = type_get_kind(t);
  if (kind == TYPE_KIND_BOOL) return true;
  if (kind >= TYPE_KIND_I8 && kind <= TYPE_KIND_U64) return true;
  if (kind >= TYPE_KIND_F16 && kind <= TYPE_KIND_F64) return true;
  if (kind == TYPE_KIND_STR) return true;
  if (kind == TYPE_KIND_ENUM) {
    type_t underlying = enum_type_get_underlying((enum_type_t)t);
    return generic_param_is_value_type_allowed(underlying);
  }
  return false;
}

/* ---- generic_instance_t class ---- */

static void _generic_instance_init(void *self, allocator_t allocator, void *arg) {
  generic_instance_t gi = (generic_instance_t)self;
  generic_instance_init_t *init = (generic_instance_init_t *)arg;
  (void)allocator;

  /* Move semantics: take ownership of the params vec and instance value directly.
   * The caller must NOT dispose them after creating the cache entry. */
  gi->params   = init->params;
  gi->instance = init->instance;
}

static void _generic_instance_dispose(void *self, allocator_t allocator) {
  generic_instance_t gi = (generic_instance_t)self;
  /* params vec is auto_dispose=false: it holds borrowed argv value pointers
   * owned by the caller's scope. Freeing the vec only releases the vec
   * structure + its pointer array, NOT the values themselves. */
  allocator_free(allocator, &gi->params);
  /* instance is borrowed from gt_scope->values (owned by the generic's
   * isolated scope). Do NOT free here — gt_scope_dispose will free it. */
  gi->instance = NULL;
}

static void _generic_instance_clone(void *self, allocator_t allocator, void *another) {
  generic_instance_t dst = (generic_instance_t)self;
  (void)another; /* cache entries are not cloned — src is irrelevant */

  /* clone params: value_clone needs vm_t which we don't have in class clone.
   * Cache entries are never cloned (generic types clone with empty cache). */
  vec_init_t vi = {.auto_dispose = false};
  dst->params = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  /* instance clone also skipped — cache entries are not cloned */
  dst->instance = NULL;
}

class_t g_generic_instance_class = {
    .size    = sizeof(struct _generic_instance_t),
    .name    = "cubec.engine.generic_instance",
    .init    = (class_init_fn_t)_generic_instance_init,
    .dispose = (class_dispose_fn_t)_generic_instance_dispose,
    .clone   = (class_clone_fn_t)_generic_instance_clone,
    .move    = NULL,
};

/* ---- generic_instance_t public API ---- */

generic_instance_t generic_instance_create(allocator_t allocator,
                                           vec_t params, value_t instance) {
  generic_instance_init_t init = {
      .params   = params,
      .instance = instance,
  };
  return (generic_instance_t)allocator_create(allocator, &g_generic_instance_class, &init);
}

vec_t   generic_instance_get_params(generic_instance_t self) { return self->params; }
value_t generic_instance_get_instance(generic_instance_t self) { return self->instance; }
