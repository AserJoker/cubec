#include "engine/generic_fn_type.h"
#include "engine/type.h"
#include "engine/scope.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/exception_type.h"
#include "engine/generic_param.h"
#include "core/string.h"
#include <stdbool.h>

/* ---- Internal init args (not exposed in header) ---- */

typedef struct generic_fn_type_init_t {
  type_kind_t kind;
  const char *name;
  uint64_t    size;
  uint64_t    align;
  bool        mut;
  vtable_t    vtable;
  vec_t       params;      /* borrowed vec of generic_param_t, each will be alloc_clone'd */
  void       *node;        /* borrowed: AST node reference */
} generic_fn_type_init_t;

/* ---- Forward declarations ---- */

static value_t _generic_fn_instantiate(vm_t vm, value_t self, size_t argc, value_t *argv);

static type_t _generic_fn_type_type_clone(vm_t vm, type_t self) {
  generic_fn_type_t src = (generic_fn_type_t)self;
  allocator_t allocator = vm_get_allocator(vm);
  generic_fn_type_t dst = (generic_fn_type_t)allocator_create(allocator, &g_generic_fn_type_class, NULL);

  dst->base.kind   = src->base.kind;
  dst->base.name   = src->base.name ? cstring_clone(allocator, src->base.name) : NULL;
  dst->base.size   = src->base.size;
  dst->base.align  = src->base.align;
  dst->base.mut    = src->base.mut;
  dst->base.vtable = src->base.vtable;

  /* clone params (each generic_param_t alloc_clone'd — owned class objects) */
  vec_init_t vi = {.auto_dispose = true};
  dst->params = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  size_t n = vec_get_size(src->params);
  for (size_t i = 0; i < n; i++) {
    generic_param_t p = (generic_param_t)vec_get(src->params, i);
    generic_param_t cloned = (generic_param_t)alloc_clone(allocator, p);
    vec_push(dst->params, cloned);
  }

  /* instances: start empty (cache is not cloned) */
  dst->instances = (vec_t)allocator_create(allocator, &g_vec_class, &vi);

  /* clone isolated scope */
  dst->scope = scope_create(allocator, SCOPE_TYPE, NULL, NULL);

  /* node: borrowed, shallow copy */
  dst->node = src->node;

  vec_push(vm_get_types(vm), dst);
  return (type_t)dst;
}

/* ---- Shared vtable factory ---- */

static vtable_t _make_generic_fn_vtable(void) {
  return (vtable_t){
      .instantiate = _generic_fn_instantiate,
      .type_clone  = _generic_fn_type_type_clone,
  };
}

/* ---- generic_fn_type_t class ---- */

static void _generic_fn_type_init(void *self, allocator_t allocator, void *arg) {
  generic_fn_type_t gt = (generic_fn_type_t)self;
  generic_fn_type_init_t *init = (generic_fn_type_init_t *)arg;

  /* init base */
  gt->base.kind   = init->kind;
  gt->base.name   = init->name ? cstring_clone(allocator, init->name) : NULL;
  gt->base.size   = init->size;
  gt->base.align  = init->align;
  gt->base.mut    = init->mut;
  gt->base.vtable = init->vtable;

  /* deep-copy params (each generic_param_t alloc_clone'd) */
  vec_init_t vi = {.auto_dispose = true};
  gt->params = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  if (init->params) {
    size_t n = vec_get_size(init->params);
    for (size_t i = 0; i < n; i++) {
      generic_param_t p = (generic_param_t)vec_get(init->params, i);
      generic_param_t cloned = (generic_param_t)alloc_clone(allocator, p);
      vec_push(gt->params, cloned);
    }
  }

  /* instances cache: auto_dispose=true vec of generic_instance_t */
  gt->instances = (vec_t)allocator_create(allocator, &g_vec_class, &vi);

  /* isolated scope — owns lifecycle of instantiated types/values */
  gt->scope = scope_create(allocator, SCOPE_TYPE, NULL, NULL);

  gt->node = init->node;
}

static void _generic_fn_type_dispose(void *self, allocator_t allocator) {
  generic_fn_type_t gt = (generic_fn_type_t)self;

  /* dispose instances cache (auto_dispose=true, owns generic_instance_t) */
  allocator_free(allocator, &gt->instances);

  /* dispose isolated scope (owns instantiated types/values) */
  if (gt->scope) {
    scope_dispose(gt->scope);
    gt->scope = NULL;
  }

  /* dispose params (auto_dispose=true, owns generic_param_t) */
  allocator_free(allocator, &gt->params);

  if (gt->base.name) {
    void *p = gt->base.name;
    allocator_free(allocator, &p);
    gt->base.name = NULL;
  }

  gt->node = NULL;
}

class_t g_generic_fn_type_class = {
    .size    = sizeof(struct _generic_fn_type_t),
    .name    = "cubec.engine.generic_fn_type",
    .init    = (class_init_fn_t)_generic_fn_type_init,
    .dispose = (class_dispose_fn_t)_generic_fn_type_dispose,
    .clone   = NULL, /* types are global singletons — use vtable.type_clone instead */
    .move    = NULL,
};

/* ---- _generic_fn_instantiate vtable implementation ---- */

static value_t _generic_fn_instantiate(vm_t vm, value_t self, size_t argc, value_t *argv) {
  type_t self_type = value_get_type(self);
  generic_fn_type_t gt = (generic_fn_type_t)self_type;

  /* 1. Validate argc against param count, accounting for pack params.
   * A pack param (is_rest=true) absorbs all remaining arguments, so:
   * - argc must be >= number of non-rest params
   * - only the last param can be a pack param */
  vec_t param_defs = generic_fn_type_get_params(gt);
  size_t param_count = vec_get_size(param_defs);
  size_t non_rest_count = 0;
  bool has_rest = false;
  for (size_t i = 0; i < param_count; i++) {
    generic_param_t gp = (generic_param_t)vec_get(param_defs, i);
    if (generic_param_is_rest(gp)) {
      has_rest = true;
    } else {
      non_rest_count++;
    }
  }
  if (has_rest) {
    if (argc < non_rest_count)
      return create_exception_value(vm,
          "generic fn '%s' expects at least %zu params, got %zu",
          type_get_name(self_type), non_rest_count, argc);
  } else {
    if (argc != param_count)
      return create_exception_value(vm,
          "generic fn '%s' expects %zu params, got %zu",
          type_get_name(self_type), param_count, argc);
  }

  /* 2. extends constraint validation.
   * For non-rest params, validate argv[i] against constraints as before.
   * For the rest param, validate each absorbed argument against constraints. */
  allocator_t allocator = vm_get_allocator(vm);
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
  size_t argv_idx = 0;
  for (size_t i = 0; i < param_count; i++) {
    generic_param_t gp = (generic_param_t)vec_get(param_defs, i);
    vec_t ext = generic_param_get_extends(gp);
    size_t ext_count = vec_get_size(ext);
    if (ext_count == 0) {
      /* no constraints — just advance argv index */
      if (generic_param_is_rest(gp)) {
        /* rest param absorbs remaining args */
        argv_idx = argc;
      } else {
        argv_idx++;
      }
      continue;
    }
    if (generic_param_is_rest(gp)) {
      /* rest param: validate each absorbed argument */
      for (size_t k = argv_idx; k < argc; k++) {
        for (size_t j = 0; j < ext_count; j++) {
          type_t constraint = (type_t)vec_get(ext, j);
          value_t constraint_val = value_create(allocator, type_type, constraint, false);
          value_t ext_result = value_extends(vm, argv[k], constraint_val);
          allocator_free(allocator, &constraint_val);
          if (value_is_abnormal(ext_result)) return ext_result;
          bool ok = *(bool *)value_get_data(ext_result);
          if (!ok)
            return create_exception_value(vm,
                "generic param '%s' constraint not satisfied for argument %zu",
                generic_param_get_name(gp), k);
        }
      }
      argv_idx = argc;
    } else {
      /* normal param: validate single argument */
      for (size_t j = 0; j < ext_count; j++) {
        type_t constraint = (type_t)vec_get(ext, j);
        value_t constraint_val = value_create(allocator, type_type, constraint, false);
        value_t ext_result = value_extends(vm, argv[argv_idx], constraint_val);
        allocator_free(allocator, &constraint_val);
        if (value_is_abnormal(ext_result)) return ext_result;
        bool ok = *(bool *)value_get_data(ext_result);
        if (!ok)
          return create_exception_value(vm,
              "generic param '%s' constraint not satisfied",
              generic_param_get_name(gp));
      }
      argv_idx++;
    }
  }

  /* 3. Delegate to create_instance callback from value.data */
  create_instance_fn_t fn = (create_instance_fn_t)value_get_data(self);
  if (!fn)
    return create_exception_value(vm, "generic fn '%s' has no instantiation callback",
                                  type_get_name(self_type));
  return fn(vm, self, argc, argv);
}

/* ---- Type creation ---- */

generic_fn_type_t generic_fn_type_create(allocator_t allocator, const char *name,
                                         vec_t params, void *node) {
  generic_fn_type_init_t init = {
      .kind   = TYPE_KIND_GENERIC_FN,
      .name   = name,
      .size   = 0,
      .align  = 0,
      .mut    = false,
      .vtable = _make_generic_fn_vtable(),
      .params = params,
      .node   = node,
  };
  return (generic_fn_type_t)allocator_create(allocator, &g_generic_fn_type_class, &init);
}

/* ---- Accessors ---- */

vec_t generic_fn_type_get_params(generic_fn_type_t self) { return self->params; }
vec_t generic_fn_type_get_instances(generic_fn_type_t self) { return self->instances; }
struct _scope_t *generic_fn_type_get_scope(generic_fn_type_t self) { return self->scope; }
void *generic_fn_type_get_node(generic_fn_type_t self) { return self->node; }
