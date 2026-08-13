#include "engine/vm.h"
#include "engine/context.h"
#include "engine/scope.h"
#include "engine/type.h"
#include "engine/exception_type.h"
#include "engine/bool_type.h"
#include "engine/wildcard_type.h"
#include "engine/void_type.h"
#include "engine/integer_type.h"
#include "engine/float_type.h"
#include "engine/str_type.h"
#include "engine/array_type.h"
#include "engine/slice_type.h"
#include "engine/tuple_type.h"
#include "engine/callable_type.h"
#include "engine/pointer_type.h"
#include "engine/union_type.h"
#include "engine/struct_type.h"
#include "engine/module.h"
#include "core/string.h"
#include "core/strmap.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include <stdio.h>
#include <string.h>

/* ---- Builtin callable: panic ---- */

static value_t _builtin_panic(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)fn;
  (void)argc;
  (void)argv;
  /* extract message from first argument (str) */
  if (argc < 1)
    return create_exception_value(vm, "panic expected 1 argument, got 0");
  value_t msg_val = argv[0];
  if (type_get_kind(value_get_type(msg_val)) != TYPE_KIND_STR)
    return create_exception_value(vm, "panic expected str argument");
  /* read the string content */
  string_t s = *(string_t *)value_get_data(msg_val);
  return create_exception_value(vm, "panic: %s", string_get(s));
}

/* ---- Builtin result[T,E] methods ---- */

/** ok(*const result[T,E]) -> bool
 *  Returns true if the active variant is _value (tag=0). */
static value_t _result_ok(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)fn;
  (void)argc;
  if (argc < 1)
    return create_exception_value(vm, "result.ok expected 1 argument");
  /* argv[0] = *const result[T,E] (pointer from member_call) */
  value_t self = value_deref_get(vm, argv[0]);
  if (type_get_kind(value_get_type(self)) == TYPE_KIND_EXCEPTION)
    return self;
  union_type_t ut = (union_type_t)value_get_type(self);
  type_t T = field_info_get_type(union_type_find_field(ut, "_value"));
  return value_is(vm, self, T);
}

/** value(*const result[T,E]) -> T
 *  Returns _value if ok, panics otherwise. */
static value_t _result_value(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)fn;
  (void)argc;
  if (argc < 1)
    return create_exception_value(vm, "result.value expected 1 argument");
  value_t self = value_deref_get(vm, argv[0]);
  if (type_get_kind(value_get_type(self)) == TYPE_KIND_EXCEPTION)
    return self;
  union_type_t ut = (union_type_t)value_get_type(self);
  type_t T = field_info_get_type(union_type_find_field(ut, "_value"));
  value_t is_ok = value_is(vm, self, T);
  if (type_get_kind(value_get_type(is_ok)) == TYPE_KIND_EXCEPTION)
    return is_ok;
  if (!*(bool *)value_get_data(is_ok))
    return create_exception_value(vm, "result.value called on error variant");
  return value_get_field(vm, self, "_value");
}

/** error(*const result[T,E]) -> E
 *  Returns _error if !ok, panics otherwise. */
static value_t _result_error(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)fn;
  (void)argc;
  if (argc < 1)
    return create_exception_value(vm, "result.error expected 1 argument");
  value_t self = value_deref_get(vm, argv[0]);
  if (type_get_kind(value_get_type(self)) == TYPE_KIND_EXCEPTION)
    return self;
  union_type_t ut = (union_type_t)value_get_type(self);
  type_t T = field_info_get_type(union_type_find_field(ut, "_value"));
  value_t is_ok = value_is(vm, self, T);
  if (type_get_kind(value_get_type(is_ok)) == TYPE_KIND_EXCEPTION)
    return is_ok;
  if (*(bool *)value_get_data(is_ok))
    return create_exception_value(vm, "result.error called on ok variant");
  return value_get_field(vm, self, "_error");
}

/** of_value(T) -> result[T,E]  (static constructor)
 *  Uses return type of callable to recover union_type_t. */
static value_t _result_of_value(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)argc;
  if (argc < 1)
    return create_exception_value(vm, "result.of_value expected 1 argument");
  callable_type_t ct = (callable_type_t)value_get_type(fn);
  union_type_t ut = (union_type_t)callable_type_get_return_type(ct);
  type_t T = field_info_get_type(union_type_find_field(ut, "_value"));
  value_t val = value_safe_cast(vm, argv[0], T);
  if (type_get_kind(value_get_type(val)) == TYPE_KIND_EXCEPTION)
    return val;
  return create_union_value(vm, ut, 0, val);
}

/** of_error(E) -> result[T,E]  (static constructor)
 *  Uses return type of callable to recover union_type_t. */
static value_t _result_of_error(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)argc;
  if (argc < 1)
    return create_exception_value(vm, "result.of_error expected 1 argument");
  callable_type_t ct = (callable_type_t)value_get_type(fn);
  union_type_t ut = (union_type_t)callable_type_get_return_type(ct);
  type_t E = field_info_get_type(union_type_find_field(ut, "_error"));
  value_t err = value_safe_cast(vm, argv[0], E);
  if (type_get_kind(value_get_type(err)) == TYPE_KIND_EXCEPTION)
    return err;
  return create_union_value(vm, ut, 1, err);
}

struct _vm_t {
  allocator_t allocator;
  strmap_t    modules;       /* absolute path → module_t (auto-dispose) */
  scope_t     global_scope;  /* owned: global scope */
  scope_t     root_scope;    /* borrowed: current module's root scope */
  scope_t     current_scope; /* borrowed: current traversal position */
  value_t     v_type;        /* borrowed: bootstrap type "type" (in global_scope->values) */
  value_t     v_exception;   /* borrowed: bootstrap type "exception" (in global_scope->values) */
  value_t     v_bool;        /* borrowed: bootstrap type "bool" (in global_scope->values) */
  value_t     v_wildcard;    /* borrowed: bootstrap type "wildcard" (in global_scope->values) */
  value_t     v_void;        /* borrowed: bootstrap type "void" (in global_scope->values) */
  value_t     v_const_bool;  /* borrowed: bootstrap type "const bool" (in global_scope->values) */
  value_t     v_i8;          /* borrowed: bootstrap type "i8" */
  value_t     v_i16;         /* borrowed: bootstrap type "i16" */
  value_t     v_i32;         /* borrowed: bootstrap type "i32" */
  value_t     v_i64;         /* borrowed: bootstrap type "i64" */
  value_t     v_const_i8;    /* borrowed: bootstrap type "const i8" */
  value_t     v_const_i16;   /* borrowed: bootstrap type "const i16" */
  value_t     v_const_i32;   /* borrowed: bootstrap type "const i32" */
  value_t     v_const_i64;   /* borrowed: bootstrap type "const i64" */
  value_t     v_u8;          /* borrowed: bootstrap type "u8" */
  value_t     v_u16;         /* borrowed: bootstrap type "u16" */
  value_t     v_u32;         /* borrowed: bootstrap type "u32" */
  value_t     v_u64;         /* borrowed: bootstrap type "u64" */
  value_t     v_const_u8;    /* borrowed: bootstrap type "const u8" */
  value_t     v_const_u16;   /* borrowed: bootstrap type "const u16" */
  value_t     v_const_u32;   /* borrowed: bootstrap type "const u32" */
  value_t     v_const_u64;   /* borrowed: bootstrap type "const u64" */
  value_t     v_f16;         /* borrowed: bootstrap type "f16" */
  value_t     v_f32;         /* borrowed: bootstrap type "f32" */
  value_t     v_f64;         /* borrowed: bootstrap type "f64" */
  value_t     v_const_f16;   /* borrowed: bootstrap type "const f16" */
  value_t     v_const_f32;   /* borrowed: bootstrap type "const f32" */
  value_t     v_const_f64;   /* borrowed: bootstrap type "const f64" */
  value_t     v_str;         /* borrowed: bootstrap type "str" */
  value_t     v_const_str;   /* borrowed: bootstrap type "const str" */
  value_t     v_wildcard_tuple; /* borrowed: bootstrap type "<?>" (wildcard tuple) */
  value_t     v_wildcard_value; /* borrowed: global unique wildcard value for generic params */
  value_t     v_error;         /* borrowed: user-facing error struct type value */
  const char *current_module_id; /* borrowed: current module path or "<builtin>" */
  vec_t       call_stack;      /* vec of call_frame_t (auto_dispose=true, owned name/message) */
};

static void _vm_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  vm_t vm = (vm_t)self;
  vm->allocator = allocator;
  vm->current_module_id = "<builtin>";

  strmap_init_t sm_init = {.value_auto_dispose = true};
  vm->modules = (strmap_t)allocator_create(allocator, &g_strmap_class, &sm_init);

  vm->global_scope = scope_create(allocator, SCOPE_GLOBAL, NULL, NULL);
  vm->root_scope = NULL;
  vm->current_scope = vm->global_scope;
  vm->call_stack = NULL;

  /* v_type must be created first — create_type_value depends on it.
   * Cannot use create_type_value for v_type itself (circular dependency). */
  type_t type_type = type_get_type_type(allocator);
  vec_push(vm->global_scope->types, type_type);
  vm->v_type = value_create(allocator, type_type, type_type, false);
  vec_push(vm->global_scope->values, vm->v_type);
  name_t n_type = name_create(vm->global_scope->allocator, vm->v_type);
  strmap_insert(vm->global_scope->names, "type", n_type);

  /* Subsequent builtin types use create_type_value.
   * Each type is heap-allocated and registered in global_scope->types
   * so scope dispose auto-frees them. value.data = type (ref, own=false). */
  type_t exception_type = type_get_exception_type(allocator);
  vec_push(vm->global_scope->types, exception_type);
  vm->v_exception = create_type_value(vm, exception_type, NULL, false);

  type_t bool_type = type_get_bool_type(allocator);
  vec_push(vm->global_scope->types, bool_type);
  vm->v_bool = create_type_value(vm, bool_type, "bool", false);

  type_t wildcard_type = type_get_wildcard_type(allocator);
  vec_push(vm->global_scope->types, wildcard_type);
  vm->v_wildcard = create_type_value(vm, wildcard_type, NULL, false);

  type_t void_type = type_get_void_type(allocator);
  vec_push(vm->global_scope->types, void_type);
  vm->v_void = create_type_value(vm, void_type, "void", false);

  type_t const_bool_type = type_get_const_bool_type(allocator);
  vec_push(vm->global_scope->types, const_bool_type);
  vm->v_const_bool = create_type_value(vm, const_bool_type, "const bool", false);

  /* Integer types */
  type_t i8_type = type_get_i8_type(allocator);
  vec_push(vm->global_scope->types, i8_type);
  vm->v_i8 = create_type_value(vm, i8_type, "i8", false);

  type_t i16_type = type_get_i16_type(allocator);
  vec_push(vm->global_scope->types, i16_type);
  vm->v_i16 = create_type_value(vm, i16_type, "i16", false);

  type_t i32_type = type_get_i32_type(allocator);
  vec_push(vm->global_scope->types, i32_type);
  vm->v_i32 = create_type_value(vm, i32_type, "i32", false);

  type_t i64_type = type_get_i64_type(allocator);
  vec_push(vm->global_scope->types, i64_type);
  vm->v_i64 = create_type_value(vm, i64_type, "i64", false);

  type_t const_i8_type = type_get_const_i8_type(allocator);
  vec_push(vm->global_scope->types, const_i8_type);
  vm->v_const_i8 = create_type_value(vm, const_i8_type, "const i8", false);

  type_t const_i16_type = type_get_const_i16_type(allocator);
  vec_push(vm->global_scope->types, const_i16_type);
  vm->v_const_i16 = create_type_value(vm, const_i16_type, "const i16", false);

  type_t const_i32_type = type_get_const_i32_type(allocator);
  vec_push(vm->global_scope->types, const_i32_type);
  vm->v_const_i32 = create_type_value(vm, const_i32_type, "const i32", false);

  type_t const_i64_type = type_get_const_i64_type(allocator);
  vec_push(vm->global_scope->types, const_i64_type);
  vm->v_const_i64 = create_type_value(vm, const_i64_type, "const i64", false);

  /* Unsigned integer types */
  type_t u8_type = type_get_u8_type(allocator);
  vec_push(vm->global_scope->types, u8_type);
  vm->v_u8 = create_type_value(vm, u8_type, "u8", false);

  type_t u16_type = type_get_u16_type(allocator);
  vec_push(vm->global_scope->types, u16_type);
  vm->v_u16 = create_type_value(vm, u16_type, "u16", false);

  type_t u32_type = type_get_u32_type(allocator);
  vec_push(vm->global_scope->types, u32_type);
  vm->v_u32 = create_type_value(vm, u32_type, "u32", false);

  type_t u64_type = type_get_u64_type(allocator);
  vec_push(vm->global_scope->types, u64_type);
  vm->v_u64 = create_type_value(vm, u64_type, "u64", false);

  type_t const_u8_type = type_get_const_u8_type(allocator);
  vec_push(vm->global_scope->types, const_u8_type);
  vm->v_const_u8 = create_type_value(vm, const_u8_type, "const u8", false);

  type_t const_u16_type = type_get_const_u16_type(allocator);
  vec_push(vm->global_scope->types, const_u16_type);
  vm->v_const_u16 = create_type_value(vm, const_u16_type, "const u16", false);

  type_t const_u32_type = type_get_const_u32_type(allocator);
  vec_push(vm->global_scope->types, const_u32_type);
  vm->v_const_u32 = create_type_value(vm, const_u32_type, "const u32", false);

  type_t const_u64_type = type_get_const_u64_type(allocator);
  vec_push(vm->global_scope->types, const_u64_type);
  vm->v_const_u64 = create_type_value(vm, const_u64_type, "const u64", false);

  /* Float types */
  type_t f16_type = type_get_f16_type(allocator);
  vec_push(vm->global_scope->types, f16_type);
  vm->v_f16 = create_type_value(vm, f16_type, "f16", false);

  type_t f32_type = type_get_f32_type(allocator);
  vec_push(vm->global_scope->types, f32_type);
  vm->v_f32 = create_type_value(vm, f32_type, "f32", false);

  type_t f64_type = type_get_f64_type(allocator);
  vec_push(vm->global_scope->types, f64_type);
  vm->v_f64 = create_type_value(vm, f64_type, "f64", false);

  type_t const_f16_type = type_get_const_f16_type(allocator);
  vec_push(vm->global_scope->types, const_f16_type);
  vm->v_const_f16 = create_type_value(vm, const_f16_type, "const f16", false);

  type_t const_f32_type = type_get_const_f32_type(allocator);
  vec_push(vm->global_scope->types, const_f32_type);
  vm->v_const_f32 = create_type_value(vm, const_f32_type, "const f32", false);

  type_t const_f64_type = type_get_const_f64_type(allocator);
  vec_push(vm->global_scope->types, const_f64_type);
  vm->v_const_f64 = create_type_value(vm, const_f64_type, "const f64", false);

  /* Str type */
  type_t str_type = type_get_str_type(allocator);
  vec_push(vm->global_scope->types, str_type);
  vm->v_str = create_type_value(vm, str_type, "str", false);

  type_t const_str_type = type_get_const_str_type(allocator);
  vec_push(vm->global_scope->types, const_str_type);
  vm->v_const_str = create_type_value(vm, const_str_type, "const str", false);

  /* Wildcard tuple type <?> — extends placeholder for any tuple */
  type_t wildcard_tuple_type = type_create(allocator, TYPE_KIND_TUPLE, "<?>",
      0, 0, false, (vtable_t){0});
  vec_push(vm->global_scope->types, wildcard_tuple_type);
  vm->v_wildcard_tuple = create_type_value(vm, wildcard_tuple_type, NULL, false);

  /* Wildcard value — global unique sentinel for value-type generic parameters.
   * type = wildcard_type, data = NULL, own = false.
   * Used in type_equal/type_extends: if a parameter == vm_get_wildcard_value(vm),
   * skip comparison for that parameter. */
  vm->v_wildcard_value = value_create(allocator, wildcard_type, NULL, false);
  vec_push(vm->global_scope->values, vm->v_wildcard_value);

  /* User-facing error struct: error { message: [128]u8, error_code: u64,
   *                                    backtrace: [32]u64, backtrace_count: u64 } */
  struct_type_t error_st = struct_type_create(allocator, "error", true, "<builtin>");

  /* message: [128]u8 */
  type_t err_u8_type = (type_t)value_get_data(vm->v_u8);
  type_t msg_array_type = (type_t)array_type_create(allocator, err_u8_type, 128, true);
  vec_push(vm->global_scope->types, msg_array_type);
  struct_type_add_field(allocator, error_st, "message", msg_array_type, true);

  /* error_code: u64 */
  type_t err_u64_type = (type_t)value_get_data(vm->v_u64);
  struct_type_add_field(allocator, error_st, "error_code", err_u64_type, true);

  /* backtrace: [32]u64 */
  type_t bt_array_type = (type_t)array_type_create(allocator, err_u64_type, 32, true);
  vec_push(vm->global_scope->types, bt_array_type);
  struct_type_add_field(allocator, error_st, "backtrace", bt_array_type, true);

  /* backtrace_count: u64 */
  struct_type_add_field(allocator, error_st, "backtrace_count", err_u64_type, true);

  struct_type_seal(error_st);
  vec_push(vm->global_scope->types, (type_t)error_st);
  vm->v_error = create_type_value(vm, (type_t)error_st, "error", false);

  /* ---- builtin: panic(str) -> void ---- */
  /* panic constructs an exception and returns it.
   * Declared return type is void, but _callable_call short-circuits
   * exception values before the safe_cast to return_type. */
  {
    vec_init_t vi = {.auto_dispose = false}; /* borrowed type refs */
    vec_t panic_params = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
    type_t str_t = (type_t)value_get_data(vm->v_str);
    vec_push(panic_params, str_t);

    type_t void_t = (type_t)value_get_data(vm->v_void);
    callable_type_t panic_ct = callable_type_create(allocator, panic_params,
                                                      void_t, false, true,
                                                      "<builtin>");
    allocator_free(allocator, &panic_params);
    vec_push(vm->global_scope->types, panic_ct);
    value_t panic_val = create_callable_value(vm, panic_ct, _builtin_panic,
                                               "panic");
    name_t n_panic = name_create(vm->global_scope->allocator, panic_val);
    strmap_insert(vm->global_scope->names, "panic", n_panic);
  }
}

static void _vm_dispose(void *self, allocator_t allocator) {
  vm_t vm = (vm_t)self;
  (void)allocator;
  allocator_free(vm->allocator, &vm->call_stack);
  allocator_free(vm->allocator, &vm->modules);
  allocator_free(vm->allocator, &vm->global_scope);
}

class_t g_vm_class = {
    .size = sizeof(struct _vm_t),
    .name = "cubec.engine.vm",
    .init = (class_init_fn_t)_vm_init,
    .dispose = (class_dispose_fn_t)_vm_dispose,
    .clone = NULL,
    .move = NULL,
};

vm_t vm_create(allocator_t allocator) {
  return (vm_t)allocator_create(allocator, &g_vm_class, NULL);
}

void vm_dispose(vm_t self, allocator_t allocator) {
  if (!self) return;
  allocator_free(allocator, &self);
}

allocator_t vm_get_allocator(vm_t self) { return self->allocator; }
strmap_t vm_get_modules(vm_t self) { return self->modules; }
scope_t  vm_get_global_scope(vm_t self) { return self->global_scope; }
scope_t  vm_get_root_scope(vm_t self) { return self->root_scope; }
scope_t  vm_get_current_scope(vm_t self) { return self->current_scope; }
value_t  vm_get_type_type(vm_t self) { return self->v_type; }
value_t  vm_get_exception_type(vm_t self) { return self->v_exception; }

const char *vm_get_current_module_id(vm_t self) { return self->current_module_id; }
void        vm_set_current_module_id(vm_t self, const char *module_id) {
  self->current_module_id = module_id;
}
value_t  vm_get_error_type(vm_t self) { return self->v_error; }
value_t  vm_get_bool_type(vm_t self) { return self->v_bool; }
value_t  vm_get_wildcard_type(vm_t self) { return self->v_wildcard; }
value_t  vm_get_void_type(vm_t self) { return self->v_void; }
value_t  vm_get_const_bool_type(vm_t self) { return self->v_const_bool; }
value_t  vm_get_i8_type(vm_t self)  { return self->v_i8; }
value_t  vm_get_i16_type(vm_t self) { return self->v_i16; }
value_t  vm_get_i32_type(vm_t self) { return self->v_i32; }
value_t  vm_get_i64_type(vm_t self) { return self->v_i64; }
value_t  vm_get_const_i8_type(vm_t self)  { return self->v_const_i8; }
value_t  vm_get_const_i16_type(vm_t self) { return self->v_const_i16; }
value_t  vm_get_const_i32_type(vm_t self) { return self->v_const_i32; }
value_t  vm_get_const_i64_type(vm_t self) { return self->v_const_i64; }
value_t  vm_get_u8_type(vm_t self)  { return self->v_u8; }
value_t  vm_get_u16_type(vm_t self) { return self->v_u16; }
value_t  vm_get_u32_type(vm_t self) { return self->v_u32; }
value_t  vm_get_u64_type(vm_t self) { return self->v_u64; }
value_t  vm_get_const_u8_type(vm_t self)  { return self->v_const_u8; }
value_t  vm_get_const_u16_type(vm_t self) { return self->v_const_u16; }
value_t  vm_get_const_u32_type(vm_t self) { return self->v_const_u32; }
value_t  vm_get_const_u64_type(vm_t self) { return self->v_const_u64; }
value_t  vm_get_f16_type(vm_t self) { return self->v_f16; }
value_t  vm_get_f32_type(vm_t self) { return self->v_f32; }
value_t  vm_get_f64_type(vm_t self) { return self->v_f64; }
value_t  vm_get_const_f16_type(vm_t self) { return self->v_const_f16; }
value_t  vm_get_const_f32_type(vm_t self) { return self->v_const_f32; }
value_t  vm_get_const_f64_type(vm_t self) { return self->v_const_f64; }
value_t  vm_get_str_type(vm_t self) { return self->v_str; }
value_t  vm_get_const_str_type(vm_t self) { return self->v_const_str; }
value_t  vm_get_wildcard_tuple_type(vm_t self) { return self->v_wildcard_tuple; }
value_t  vm_get_wildcard_value(vm_t self) { return self->v_wildcard_value; }

module_t vm_get_module(vm_t self, const char *abs_path) {
  return (module_t)strmap_find(self->modules, abs_path);
}

/* ---- File I/O ---- */

static char *_read_file(allocator_t allocator, const char *path, size_t *out_len) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = (char *)allocator_alloc(allocator, (size_t)len + 1);
  if (!buf) {
    fclose(f);
    return NULL;
  }
  size_t n = fread(buf, 1, (size_t)len, f);
  buf[n] = '\0';
  fclose(f);
  if (out_len)
    *out_len = n;
  return buf;
}

/* ---- Path resolution ---- */

static char *_resolve_import_path(vm_t vm, const char *import_path) {
  if (!import_path || import_path[0] == '\0')
    return NULL;

  bool is_relative =
      (import_path[0] == '.' &&
       (import_path[1] == '/' || (import_path[1] == '.' && import_path[2] == '/')));

  bool is_absolute = (import_path[0] == '/' || import_path[0] == '\\'
#ifdef _WIN32
                      || (import_path[0] && import_path[1] == ':')
#endif
  );

  if (!is_relative && !is_absolute)
    return NULL;

  char *resolved = NULL;

  if (is_relative) {
    if (vm->root_scope && vm->root_scope->owner) {
      module_t current_mod = (module_t)vm->root_scope->owner;
      const char *current_file = current_mod->filename;
      if (current_file) {
        const char *last_slash = strrchr(current_file, '/');
#ifdef _WIN32
        const char *last_backslash = strrchr(current_file, '\\');
        if (!last_slash || (last_backslash && last_backslash > last_slash))
          last_slash = last_backslash;
#endif
        if (last_slash) {
          size_t dir_len = (size_t)(last_slash - current_file) + 1;
          size_t path_len = strlen(import_path);
          resolved = (char *)allocator_alloc(vm->allocator, dir_len + path_len + 1);
          if (!resolved)
            return NULL;
          memcpy(resolved, current_file, dir_len);
          memcpy(resolved + dir_len, import_path, path_len);
          resolved[dir_len + path_len] = '\0';
        }
      }
    }
  }

  if (!resolved) {
    size_t path_len = strlen(import_path);
    bool has_ext = (path_len > 6 && strcmp(import_path + path_len - 6, ".cubec") == 0);
    size_t ext_len = has_ext ? 0 : 6;
    resolved = (char *)allocator_alloc(vm->allocator, path_len + ext_len + 1);
    if (!resolved)
      return NULL;
    memcpy(resolved, import_path, path_len);
    if (!has_ext) {
      memcpy(resolved + path_len, ".cubec", 6);
    }
    resolved[path_len + ext_len] = '\0';
  }

#ifdef _WIN32
  char abs_buf[_MAX_PATH];
  char *abs = _fullpath(abs_buf, resolved, _MAX_PATH);
  char *result = abs ? cstring_clone(vm->allocator, abs) : NULL;
#else
  char real_buf[4096];
  char *real = realpath(resolved, real_buf);
  char *result = real ? cstring_clone(vm->allocator, real) : NULL;
#endif
  allocator_free(vm->allocator, (void **)&resolved);
  return result;
}

/* ---- vm_import ---- */

module_t vm_import(vm_t self, context_t ctx, const char *import_path) {
  char *abs_path = _resolve_import_path(self, import_path);
  if (!abs_path)
    return NULL;

  module_t existing = vm_get_module(self, abs_path);
  if (existing) {
    allocator_free(self->allocator, (void **)&abs_path);
    return existing;
  }

  char *source = _read_file(self->allocator, abs_path, NULL);
  if (!source) {
    allocator_free(self->allocator, (void **)&abs_path);
    return NULL;
  }

  vec_t tokens = resolve_token_list(ctx, abs_path, source);
  if (!tokens) {
    allocator_free(self->allocator, (void **)&source);
    allocator_free(self->allocator, (void **)&abs_path);
    return NULL;
  }

  size_t pos = 0;
  node_t program = read_program_node(ctx, tokens, &pos, abs_path);
  if (!program) {
    allocator_free(self->allocator, &tokens);
    allocator_free(self->allocator, (void **)&source);
    allocator_free(self->allocator, (void **)&abs_path);
    return NULL;
  }

  module_t mod = module_create(self->allocator, self->global_scope, abs_path,
                               source, tokens, program);

  strmap_insert(self->modules, abs_path, mod);

  allocator_free(self->allocator, (void **)&abs_path);
  return mod;
}

/* ---- Scope management ---- */

void vm_push_scope(vm_t self, scope_t scope) {
  if (!self || !scope)
    return;
  self->root_scope = scope;
  self->current_scope = scope;
}

void vm_pop_scope(vm_t self) {
  if (!self || !self->current_scope)
    return;
  self->current_scope = self->current_scope->parent;
}

scope_t vm_set_scope(vm_t self, scope_t scope) {
  if (!self)
    return NULL;
  scope_t prev = self->current_scope;
  self->current_scope = scope;
  return prev;
}

scope_t vm_set_root_scope(vm_t self, scope_t scope) {
  if (!self)
    return NULL;
  scope_t prev = self->root_scope;
  self->root_scope = scope;
  return prev;
}

/* ---- Call frame class ---- */

static void _call_frame_init(void *self, allocator_t allocator, void *arg) {
  call_frame_t cf = (call_frame_t)self;
  call_frame_init_t *init = (call_frame_init_t *)arg;
  cf->name = (init && init->name) ? cstring_clone(allocator, init->name) : NULL;
  cf->message = (init && init->message) ? cstring_clone(allocator, init->message) : NULL;
}

static void _call_frame_dispose(void *self, allocator_t allocator) {
  call_frame_t cf = (call_frame_t)self;
  if (cf->name) {
    void *p = cf->name;
    allocator_free(allocator, &p);
    cf->name = NULL;
  }
  if (cf->message) {
    void *p = cf->message;
    allocator_free(allocator, &p);
    cf->message = NULL;
  }
}

static void _call_frame_clone(void *self, allocator_t allocator, void *another) {
  call_frame_t dst = (call_frame_t)self;
  call_frame_t src = (call_frame_t)another;
  dst->name = src->name ? cstring_clone(allocator, src->name) : NULL;
  dst->message = src->message ? cstring_clone(allocator, src->message) : NULL;
}

class_t g_call_frame_class = {
    .size = sizeof(struct _call_frame_t),
    .name = "cubec.engine.call_frame",
    .init = (class_init_fn_t)_call_frame_init,
    .dispose = (class_dispose_fn_t)_call_frame_dispose,
    .clone = (class_clone_fn_t)_call_frame_clone,
    .move = NULL,
};

/* ---- Call stack ---- */

void vm_push_frame(vm_t self, const char *name, const char *message) {
  if (!self->call_stack) {
    vec_init_t vi = {.auto_dispose = true};
    self->call_stack = (vec_t)allocator_create(self->allocator, &g_vec_class, &vi);
  }
  call_frame_init_t init = {.name = name, .message = message};
  call_frame_t frame = (call_frame_t)allocator_create(self->allocator,
                                                       &g_call_frame_class, &init);
  vec_push(self->call_stack, frame);
}

void vm_pop_frame(vm_t self) {
  if (!self->call_stack)
    return;
  size_t sz = vec_get_size(self->call_stack);
  if (sz > 0)
    vec_pop(self->call_stack);
}

vec_t vm_get_call_stack(vm_t self) {
  return self->call_stack;
}

/* ---- Value creation ---- */

value_t vm_create_value(vm_t self, type_t type, const void *data,
                        const char *name) {
  size_t sz = type_get_size(type);
  void *data_copy = NULL;
  if (sz > 0) {
    data_copy = allocator_alloc(self->allocator, sz);
    if (data) {
      memcpy(data_copy, data, sz);
    } else {
      memset(data_copy, 0, sz);
    }
  }
  value_t v = value_create(self->allocator, type, data_copy, true);
  if (self->current_scope) {
    vec_push(self->current_scope->values, v);
    if (name) {
      name_t n = name_create(self->current_scope->allocator, v);
      char *owned_name = cstring_clone(self->current_scope->allocator, name);
      strmap_insert(self->current_scope->names, owned_name, n);
      allocator_free(self->current_scope->allocator, &owned_name);
    }
  }
  return v;
}

value_t vm_create_value_shadow(vm_t self, type_t type, const char *name,
                               bool initialized) {
  value_t v = value_create(self->allocator, type, NULL, false);
  value_set_initialized(v, initialized);
  if (self->current_scope) {
    vec_push(self->current_scope->values, v);
    if (name) {
      name_t n = name_create(self->current_scope->allocator, v);
      char *owned_name = cstring_clone(self->current_scope->allocator, name);
      strmap_insert(self->current_scope->names, owned_name, n);
      allocator_free(self->current_scope->allocator, &owned_name);
    }
  }
  return v;
}

value_t vm_create_array_type_value(vm_t self, type_t element_type,
                              uint64_t count, bool mut) {
  array_type_t at = array_type_create(self->allocator, element_type, count, mut);
  /* register array_type_t in scope->types for auto-dispose */
  if (self->current_scope)
    vec_push(self->current_scope->types, at);
  /* wrap as a type value — own=false because scope->types owns the type_t */
  return create_type_value(self, (type_t)at, NULL, false);
}

value_t vm_create_slice_type_value(vm_t self, type_t element_type, bool mut) {
  slice_type_t st = slice_type_create(self->allocator, element_type, mut);
  /* register slice_type_t in scope->types for auto-dispose */
  if (self->current_scope)
    vec_push(self->current_scope->types, st);
  /* wrap as a type value — own=false because scope->types owns the type_t */
  return create_type_value(self, (type_t)st, NULL, false);
}

value_t vm_create_tuple_type_value(vm_t self, vec_t element_types, bool mut) {
  tuple_type_t tt = tuple_type_create(self->allocator, element_types, mut);
  /* register tuple_type_t in scope->types for auto-dispose */
  if (self->current_scope)
    vec_push(self->current_scope->types, tt);
  /* wrap as a type value — own=false because scope->types owns the type_t */
  return create_type_value(self, (type_t)tt, NULL, false);
}

value_t vm_create_callable_type_value(vm_t self, vec_t param_types,
                                       type_t return_type, bool is_variadic,
                                       bool mut, const char *module_id) {
  callable_type_t ct = callable_type_create(self->allocator, param_types,
                                             return_type, is_variadic, mut,
                                             module_id);
  if (self->current_scope)
    vec_push(self->current_scope->types, ct);
  return create_type_value(self, (type_t)ct, NULL, false);
}

value_t vm_create_pointer_type_value(vm_t self, type_t pointee_type,
                                      bool mut, bool is_volatile) {
  pointer_type_t pt = pointer_type_create(self->allocator, pointee_type,
                                           mut, is_volatile);
  if (self->current_scope)
    vec_push(self->current_scope->types, pt);
  return create_type_value(self, (type_t)pt, NULL, false);
}

value_t vm_create_result_type_value(vm_t self, type_t T, type_t E) {
  allocator_t alloc = self->allocator;
  const char *module_id = vm_get_current_module_id(self);

  /* generate name: result[T_name,E_name] */
  const char *tname = type_get_name(T);
  const char *ename = type_get_name(E);
  size_t name_len = 7 + strlen(tname) + 1 + strlen(ename) + 1; /* "result[,,,]" */
  char *name = (char *)allocator_alloc(alloc, name_len + 1);
  snprintf(name, name_len + 1, "result[%s,%s]", tname, ename);

  /* create union type with _value:T, _error:E */
  union_type_t ut = union_type_create(alloc, name, true, module_id);
  union_type_add_field(alloc, ut, "_value", T, false); /* private */
  union_type_add_field(alloc, ut, "_error", E, false); /* private */
  union_type_seal(ut);
  allocator_free(alloc, &name); /* union_type_create cloned it */

  if (self->current_scope)
    vec_push(self->current_scope->types, ut);

  /* ---- Register methods ---- */

  /* *const result[T,E] type — used as self parameter for instance methods */
  pointer_type_t const_result_ptr = pointer_type_create(alloc, (type_t)ut, false, false);
  if (self->current_scope)
    vec_push(self->current_scope->types, const_result_ptr);

  type_t bool_t = (type_t)value_get_data(self->v_bool);

  /* ok(*const result) -> bool */
  {
    vec_init_t vi = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(params, (type_t)const_result_ptr);
    callable_type_t ct = callable_type_create(alloc, params, bool_t, false, true,
                                               module_id);
    allocator_free(alloc, &params);
    if (self->current_scope)
      vec_push(self->current_scope->types, ct);
    value_t fn = create_callable_value(self, ct, _result_ok, "ok");
    union_type_add_prop(self, ut, "ok", fn, true, true); /* method, pub */
  }

  /* value(*const result) -> T */
  {
    vec_init_t vi = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(params, (type_t)const_result_ptr);
    callable_type_t ct = callable_type_create(alloc, params, T, false, true,
                                               module_id);
    allocator_free(alloc, &params);
    if (self->current_scope)
      vec_push(self->current_scope->types, ct);
    value_t fn = create_callable_value(self, ct, _result_value, "value");
    union_type_add_prop(self, ut, "value", fn, true, true);
  }

  /* error(*const result) -> E */
  {
    vec_init_t vi = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(params, (type_t)const_result_ptr);
    callable_type_t ct = callable_type_create(alloc, params, E, false, true,
                                               module_id);
    allocator_free(alloc, &params);
    if (self->current_scope)
      vec_push(self->current_scope->types, ct);
    value_t fn = create_callable_value(self, ct, _result_error, "error");
    union_type_add_prop(self, ut, "error", fn, true, true);
  }

  /* of_value(T) -> result[T,E]  (static, not method) */
  {
    vec_init_t vi = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(params, T);
    callable_type_t ct = callable_type_create(alloc, params, (type_t)ut, false, true,
                                               module_id);
    allocator_free(alloc, &params);
    if (self->current_scope)
      vec_push(self->current_scope->types, ct);
    value_t fn = create_callable_value(self, ct, _result_of_value, "of_value");
    union_type_add_prop(self, ut, "of_value", fn, false, true); /* not method, pub */
  }

  /* of_error(E) -> result[T,E]  (static, not method) */
  {
    vec_init_t vi = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(params, E);
    callable_type_t ct = callable_type_create(alloc, params, (type_t)ut, false, true,
                                               module_id);
    allocator_free(alloc, &params);
    if (self->current_scope)
      vec_push(self->current_scope->types, ct);
    value_t fn = create_callable_value(self, ct, _result_of_error, "of_error");
    union_type_add_prop(self, ut, "of_error", fn, false, true);
  }

  return create_type_value(self, (type_t)ut, NULL, false);
}
