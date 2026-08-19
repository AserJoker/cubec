#include "engine/callable_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/func.h"
#include "engine/exception_type.h"
#include "engine/void_type.h"
#include "engine/bool_type.h"
#include "engine/str_type.h"
#include "engine/type.h"
#include "core/string.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* ---- Forward declarations for vtable functions ---- */

static value_t _callable_clone(vm_t vm, value_t self);
static value_t _callable_equal(vm_t vm, value_t a, value_t b);
static value_t _callable_type_equal(vm_t vm, type_t a, type_t b);
static value_t _callable_type_extends(vm_t vm, type_t sub, type_t super);
static value_t _callable_call(vm_t vm, value_t self, size_t argc, value_t *argv);
static value_t _callable_safe_cast(vm_t vm, value_t self, type_t to);
static value_t _callable_assignment(vm_t vm, value_t lvalue, value_t rvalue);
static value_t _callable_to_string(vm_t vm, value_t self);

static vtable_t _make_callable_vtable(void) {
  return (vtable_t){
      .clone        = _callable_clone,
      .equal        = _callable_equal,
      .extends      = NULL,
      .type_equal   = _callable_type_equal,
      .type_extends = _callable_type_extends,
      .band         = NULL,
      .bor          = NULL,
      .bxor         = NULL,
      .add          = NULL,
      .sub          = NULL,
      .mul          = NULL,
      .div          = NULL,
      .mod          = NULL,
      .shl          = NULL,
      .shr          = NULL,
      .gt           = NULL,
      .lt           = NULL,
      .bnot         = NULL,
      .lnot         = NULL,
      .pos          = NULL,
      .neg          = NULL,
      .safe_cast    = _callable_safe_cast,
      .assignment   = _callable_assignment,
      .to_string    = _callable_to_string,
      .get_field    = NULL,
      .set_field    = NULL,
      .get_item     = NULL,
      .set_item     = NULL,
      .deref_get    = NULL,
      .deref_set    = NULL,
      .slice        = NULL,
      .call         = _callable_call,
      .member_call  = NULL,
      .get_prop     = NULL,
      .set_prop     = NULL,
  };
}

/* ---- Callable type class ---- */

static void _callable_type_init(void *self, allocator_t allocator, void *arg) {
  callable_type_t ct = (callable_type_t)self;
  callable_type_init_t *init = (callable_type_init_t *)arg;

  /* init base */
  ct->base.kind  = init->kind;
  ct->base.name  = init->name ? cstring_clone(allocator, init->name) : NULL;
  ct->base.size  = init->size;
  ct->base.align = init->align;
  ct->base.mut   = init->mut;
  ct->base.vtable = init->vtable;

  /* deep-copy param_types (each element alloc_clone'd) */
  vec_init_t vi = {.auto_dispose = true};
  ct->param_types = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  for (uint64_t i = 0; i < init->param_count; i++) {
    type_t elem = (type_t)vec_get(init->param_types, (size_t)i);
    type_t cloned = (type_t)alloc_clone(allocator, elem);
    vec_push(ct->param_types, cloned);
  }

  /* deep-copy return_type */
  ct->return_type = (type_t)alloc_clone(allocator, init->return_type);
  ct->param_count = init->param_count;
  ct->is_variadic = init->is_variadic;
  ct->module_id   = init->module_id;
}

static void _callable_type_dispose(void *self, allocator_t allocator) {
  callable_type_t ct = (callable_type_t)self;
  allocator_free(allocator, &ct->param_types);
  allocator_free(allocator, &ct->return_type);
  if (ct->base.name) {
    void *p = ct->base.name;
    allocator_free(allocator, &p);
    ct->base.name = NULL;
  }
}

static void _callable_type_clone(void *self, allocator_t allocator, void *another) {
  callable_type_t dst = (callable_type_t)self;
  callable_type_t src = (callable_type_t)another;

  dst->base.kind   = src->base.kind;
  dst->base.name   = src->base.name ? cstring_clone(allocator, src->base.name) : NULL;
  dst->base.size   = src->base.size;
  dst->base.align  = src->base.align;
  dst->base.mut    = src->base.mut;
  dst->base.vtable = src->base.vtable;

  vec_init_t vi = {.auto_dispose = true};
  dst->param_types = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  for (uint64_t i = 0; i < src->param_count; i++) {
    type_t elem = (type_t)vec_get(src->param_types, (size_t)i);
    type_t cloned = (type_t)alloc_clone(allocator, elem);
    vec_push(dst->param_types, cloned);
  }

  dst->return_type = (type_t)alloc_clone(allocator, src->return_type);
  dst->param_count = src->param_count;
  dst->is_variadic = src->is_variadic;
  dst->module_id   = src->module_id;
}

class_t g_callable_type_class = {
    .size    = sizeof(struct _callable_type_t),
    .name    = "cubec.engine.callable_type",
    .init    = (class_init_fn_t)_callable_type_init,
    .dispose = (class_dispose_fn_t)_callable_type_dispose,
    .clone   = (class_clone_fn_t)_callable_type_clone,
    .move    = NULL,
};

/* ---- Type creation ---- */

callable_type_t callable_type_create(allocator_t allocator, vec_t param_types,
                                      type_t return_type, bool is_variadic,
                                      bool mut, const char *module_id) {
  /* generate name: (T1, T2, ...) -> R or (T1, T2) -> R */
  size_t name_cap = 64;
  char *name = (char *)allocator_alloc(allocator, name_cap);
  size_t pos = 0;
  name[pos++] = '(';

  uint64_t pcount = param_types ? vec_get_size(param_types) : 0;
  for (uint64_t i = 0; i < pcount; i++) {
    type_t pt = (type_t)vec_get(param_types, (size_t)i);
    const char *pn = type_get_name(pt);
    size_t pn_len = strlen(pn);
    /* resize if needed */
    if (pos + pn_len + 4 >= name_cap) {
      name_cap *= 2;
      char *new_name = (char *)allocator_alloc(allocator, name_cap);
      memcpy(new_name, name, pos);
      allocator_free(allocator, &name);
      name = new_name;
    }
    memcpy(name + pos, pn, pn_len);
    pos += pn_len;
    if (i + 1 < pcount || is_variadic) {
      name[pos++] = ',';
      name[pos++] = ' ';
    }
  }

  if (is_variadic) {
    const char *varg = "...";
    size_t vlen = strlen(varg);
    if (pos + vlen + 4 >= name_cap) {
      name_cap *= 2;
      char *new_name = (char *)allocator_alloc(allocator, name_cap);
      memcpy(new_name, name, pos);
      allocator_free(allocator, &name);
      name = new_name;
    }
    memcpy(name + pos, varg, vlen);
    pos += vlen;
  }

  name[pos++] = ')';
  name[pos++] = ' ';
  name[pos++] = '-';
  name[pos++] = '>';
  name[pos++] = ' ';

  const char *rn = type_get_name(return_type);
  size_t rn_len = strlen(rn);
  if (pos + rn_len + 1 >= name_cap) {
    name_cap *= 2;
    char *new_name = (char *)allocator_alloc(allocator, name_cap);
    memcpy(new_name, name, pos);
    allocator_free(allocator, &name);
    name = new_name;
  }
  memcpy(name + pos, rn, rn_len);
  pos += rn_len;
  name[pos] = '\0';

  callable_type_init_t init = {
      .kind         = TYPE_KIND_CALLABLE,
      .name         = name,
      .size         = sizeof(func_t),
      .align        = _Alignof(func_t),
      .mut          = mut,
      .vtable       = _make_callable_vtable(),
      .param_types  = param_types,
      .return_type  = return_type,
      .param_count  = pcount,
      .is_variadic  = is_variadic,
      .module_id    = module_id,
  };

  callable_type_t ct = (callable_type_t)allocator_create(
      allocator, &g_callable_type_class, &init);

  /* free temporary name — _callable_type_init cloned it */
  allocator_free(allocator, &name);
  return ct;
}

/* ---- Accessors ---- */

type_t    callable_type_get_param_type(callable_type_t self, uint64_t index) {
  return (type_t)vec_get(self->param_types, (size_t)index);
}
type_t    callable_type_get_return_type(callable_type_t self) { return self->return_type; }
uint64_t  callable_type_get_param_count(callable_type_t self) { return self->param_count; }
bool      callable_type_is_variadic(callable_type_t self) { return self->is_variadic; }
const char *callable_type_get_module_id(callable_type_t self) { return self->module_id; }

/* ---- Value constructors ---- */

value_t create_callable_value(vm_t vm, callable_type_t ct, cfunction_t func,
                               const char *name) {
  allocator_t alloc = vm_get_allocator(vm);
  func_init_t fn_init = {.func = func, .name = name,
                          .root_scope = vm_get_root_scope(vm)};
  func_t fc = (func_t)allocator_create(alloc, &g_func_class, &fn_init);
  /* value.data = func_t (borrowed ref), scope->cfuncs owns the lifecycle */
  value_t v = value_create(alloc, (type_t)ct, fc, false);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) {
    vec_push(scope->cfuncs, fc);
    vec_push(scope->values, v);
  }
  return v;
}

value_t create_callable_shadow(vm_t vm, callable_type_t ct, bool initialized) {
  return vm_create_value_shadow(vm, (type_t)ct, NULL, initialized);
}

/* ---- Closure capture ---- */

value_t callable_capture(vm_t vm, value_t callable, const char *name) {
  /* lookup in current scope chain */
  scope_t current = vm_get_current_scope(vm);
  name_t n = scope_lookup(current, name);
  if (!n || !n->ref)
    return create_exception_value(vm, "cannot capture '%s': not found in scope",
                                  name);

  /* ensure closure scope exists — isolated scope (parent=NULL via
   * scope_create), then manually set parent=root_scope for lookup chain
   * without add_child.  func_t owns the lifecycle. */
  func_t fc = (func_t)value_get_data(callable);
  scope_t closure = func_get_closure_scope(fc);
  if (!closure) {
    allocator_t alloc = vm_get_allocator(vm);
    closure = scope_create(alloc, SCOPE_CLOSURE, NULL, NULL);
    closure->parent = func_get_root_scope(fc);
    func_set_closure_scope(fc, closure);
  }

  /* clone value into closure scope: switch scope, clone, switch back */
  scope_t prev = vm_set_scope(vm, closure);
  value_t captured = value_clone(vm, n->ref);
  vm_set_scope(vm, prev);

  /* bind name in closure scope — value_clone already registered to
   * closure->values, but name binding happens via vm_set_scope context
   * only when using vm_create_value. value_clone does NOT add name
   * binding, so we do it manually here. */
  name_t cn = name_create(closure->allocator, captured);
  char *owned_name = cstring_clone(closure->allocator, name);
  strmap_insert(closure->names, owned_name, cn);
  allocator_free(closure->allocator, &owned_name);

  return captured;
}

/* ---- VTable: clone ---- */

static value_t _callable_clone(vm_t vm, value_t self) {
  callable_type_t ct = (callable_type_t)value_get_type(self);

  /* clone the type into current scope */
  type_t cloned_type = value_type_clone(vm, (type_t)ct);
  callable_type_t dst_ct = (callable_type_t)cloned_type;

  if (value_is_shadow(self))
    return create_callable_shadow(vm, dst_ct, value_is_initialized(self));

  func_t src_fc = (func_t)value_get_data(self);

  if (!value_is_initialized(self) || src_fc->func == NULL)
    return create_callable_shadow(vm, dst_ct, value_is_initialized(self));

  /* Clone the func object via alloc_clone (preserves ast_func_t subclass).
   * func_t / ast_func_t are class objects — alloc_clone dispatches to the
   * correct class_t.clone (g_func_class or g_ast_func_class). */
  allocator_t alloc = vm_get_allocator(vm);
  func_t cloned_fc = (func_t)alloc_clone(alloc, src_fc);

  /* Create value wrapping the cloned func (borrowed ref, scope->cfuncs owns) */
  value_t v = value_create(alloc, (type_t)dst_ct, cloned_fc, false);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) {
    vec_push(scope->cfuncs, cloned_fc);
    vec_push(scope->values, v);
  }
  return v;
}

/* ---- VTable: equal ---- */

static value_t _callable_equal(vm_t vm, value_t a, value_t b) {
  type_t tb = value_get_type(b);
  if (type_get_kind(tb) != TYPE_KIND_CALLABLE)
    return create_exception_value(vm, "cannot compare callable with '%s'",
                                  type_get_name(tb));

  /* check type compatibility */
  value_t teq = _callable_type_equal(vm, value_get_type(a), tb);
  if (value_is_abnormal(teq))
    return teq;
  if (!(*(bool *)value_get_data(teq)))
    return create_exception_value(vm, "cannot compare callable '%s' with callable '%s'",
                                  type_get_name(value_get_type(a)), type_get_name(tb));

  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);

  /* same func pointer + type_equal → equal */
  func_t fa = (func_t)value_get_data(a);
  func_t fb = (func_t)value_get_data(b);
  if (fa->func != fb->func)
    return create_bool_value(vm, false);

  /* check type equality */
  vtable_t vt = type_get_vtable((type_t)(callable_type_t)value_get_type(a));
  return vt.type_equal(vm, value_get_type(a), value_get_type(b));
}

/* ---- VTable: type_equal ---- */

static value_t _callable_type_equal(vm_t vm, type_t a, type_t b) {
  if (b->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (b->kind != TYPE_KIND_CALLABLE)
    return create_bool_value(vm, false);

  callable_type_t ca = (callable_type_t)a;
  callable_type_t cb = (callable_type_t)b;

  if (ca->param_count != cb->param_count)
    return create_bool_value(vm, false);
  if (ca->is_variadic != cb->is_variadic)
    return create_bool_value(vm, false);

  /* compare param types */
  for (uint64_t i = 0; i < ca->param_count; i++) {
    type_t pa = (type_t)vec_get(ca->param_types, (size_t)i);
    type_t pb = (type_t)vec_get(cb->param_types, (size_t)i);
    if (type_get_kind(pa) == TYPE_KIND_WILDCARD || type_get_kind(pb) == TYPE_KIND_WILDCARD)
      continue;
    vtable_t pvt = type_get_vtable(pa);
    if (pvt.type_equal) {
      value_t eq = pvt.type_equal(vm, pa, pb);
      if (value_is_abnormal(eq))
        return eq;
      if (value_is_shadow(eq))
        continue;
      if (!(*(bool *)value_get_data(eq)))
        return eq;
    } else if (type_get_kind(pa) != type_get_kind(pb)) {
      return create_bool_value(vm, false);
    }
  }

  /* compare return types */
  type_t ra = ca->return_type;
  type_t rb = cb->return_type;
  if (type_get_kind(ra) != TYPE_KIND_WILDCARD && type_get_kind(rb) != TYPE_KIND_WILDCARD) {
    vtable_t rvt = type_get_vtable(ra);
    if (rvt.type_equal) {
      value_t eq = rvt.type_equal(vm, ra, rb);
      if (value_is_abnormal(eq))
        return eq;
      if (!value_is_shadow(eq) && !(*(bool *)value_get_data(eq)))
        return eq;
    } else if (type_get_kind(ra) != type_get_kind(rb)) {
      return create_bool_value(vm, false);
    }
  }

  return create_bool_value(vm, true);
}

/* ---- VTable: type_extends ---- */

static value_t _callable_type_extends(vm_t vm, type_t sub, type_t super) {
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (super->kind != TYPE_KIND_CALLABLE)
    return create_bool_value(vm, false);

  /* initial: simplified to type_equal */
  return _callable_type_equal(vm, sub, super);
}

/* ---- VTable: call ---- */

static value_t _callable_call(vm_t vm, value_t self, size_t argc, value_t *argv) {
  callable_type_t ct = (callable_type_t)value_get_type(self);

  /* shadow: return shadow of return type */
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, ct->return_type, NULL, true);

  /* argc check */
  if (ct->is_variadic) {
    if (argc < ct->param_count)
      return create_exception_value(vm, "expected at least %llu args, got %llu",
                                (unsigned long long)ct->param_count,
                                (unsigned long long)argc);
  } else {
    if (argc != ct->param_count)
      return create_exception_value(vm, "expected %llu args, got %llu",
                                (unsigned long long)ct->param_count,
                                (unsigned long long)argc);
  }

  /* safe_cast each fixed arg to declared param type */
  allocator_t alloc = vm_get_allocator(vm);
  value_t *casted = NULL;
  if (argc > 0) {
    casted = (value_t *)allocator_alloc(alloc, sizeof(value_t) * argc);
    for (uint64_t i = 0; i < ct->param_count; i++) {
      type_t param_t = (type_t)vec_get(ct->param_types, (size_t)i);
      casted[i] = value_safe_cast(vm, argv[i], param_t);
      if (value_is_abnormal(casted[i])) {
        value_t err = casted[i];
        allocator_free(alloc, &casted);
        return err;
      }
    }
    /* variadic args pass through unchanged */
    for (uint64_t i = ct->param_count; i < argc; i++)
      casted[i] = argv[i];
  }

  /* invoke — switch to callable's module context */
  const char *prev_module = vm_get_current_module_id(vm);
  vm_set_current_module_id(vm, callable_type_get_module_id(ct));

  func_t fc = (func_t)value_get_data(self);
  value_t result = fc->func(vm, self, argc, casted);

  /* restore module context */
  vm_set_current_module_id(vm, prev_module);

  if (casted)
    allocator_free(alloc, &casted);

  /* safe_cast return value to declared return type */
  if (value_is_abnormal(result))
    return result;
  result = value_safe_cast(vm, result, ct->return_type);
  return result;
}

/* ---- VTable: safe_cast ---- */

static value_t _callable_safe_cast(vm_t vm, value_t self, type_t to) {
  if (type_get_kind(to) == TYPE_KIND_CALLABLE) {
    /* TODO: structural compatibility check */
    if (value_is_shadow(self))
      return create_callable_shadow(vm, (callable_type_t)to, value_is_initialized(self));
    return self;
  }
  return self;
}

/* ---- VTable: assignment ---- */

static value_t _callable_assignment(vm_t vm, value_t lvalue, value_t rvalue) {
  type_t lt = value_get_type(lvalue);
  type_t rt = value_get_type(rvalue);
  if (type_get_kind(rt) != TYPE_KIND_CALLABLE)
    return create_exception_value(vm, "cannot assign non-callable to callable");
  /* check type compatibility */
  value_t eq = _callable_type_equal(vm, lt, rt);
  if (value_is_abnormal(eq))
    return eq;
  if (!(*(bool *)value_get_data(eq)))
    return create_exception_value(vm, "cannot assign '%s' to '%s'",
                                  type_get_name(rt), type_get_name(lt));
  if (value_is_shadow(lvalue) || value_is_shadow(rvalue)) {
    value_set_initialized(lvalue, true);
    return create_void_value(vm);
  }
  /* copy func pointer */
  func_t src_fc = (func_t)value_get_data(rvalue);
  func_t dst_fc = (func_t)value_get_data(lvalue);
  dst_fc->func = src_fc->func;
  value_set_initialized(lvalue, true);
  return create_void_value(vm);
}

/* ---- VTable: to_string ---- */

static value_t _callable_to_string(vm_t vm, value_t self) {
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, (type_t)value_get_data(vm_get_str_type(vm)), NULL, true);
  type_t t = value_get_type(self);
  const char *type_name = type_get_name(t);
  size_t len = strlen("<callable >") + strlen(type_name);
  char *buf = (char *)allocator_alloc(vm_get_allocator(vm), len + 1);
  snprintf(buf, len + 1, "<callable %s>", type_name);
  value_t result = create_str_value(vm, buf);
  allocator_free(vm_get_allocator(vm), &buf);
  return result;
}
