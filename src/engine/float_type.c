#include "engine/float_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/error_type.h"
#include "engine/void_type.h"
#include "engine/bool_type.h"
#include "engine/type.h"
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

/* Helper: create a value and register it in vm's current scope */
static value_t _float_value_create(vm_t vm, type_t type, void *data, bool own) {
  value_t v = value_create(vm_get_allocator(vm), type, data, own);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) { vec_push(scope->values, v); }
  return v;
}

/* ---- Float kind classification ---- */

static bool _is_float_kind(type_kind_t kind) {
  return kind == TYPE_KIND_F16 || kind == TYPE_KIND_F32 || kind == TYPE_KIND_F64;
}

/* ---- f16 ↔ double conversion (IEEE 754 half-precision) ---- */

static double _f16_to_double(uint16_t h) {
  uint16_t sign = (h >> 15) & 0x1;
  uint16_t exp  = (h >> 10) & 0x1F;
  uint16_t frac = h & 0x3FF;

  if (exp == 0) {
    if (frac == 0) {
      /* ±zero */
      return sign ? -0.0 : 0.0;
    }
    /* subnormal: (-1)^sign * 2^-14 * (0.frac) */
    double m = (double)frac / 1024.0;
    return sign ? -m * 6.103515625e-05 : m * 6.103515625e-05;
  }
  if (exp == 31) {
    if (frac == 0) {
      /* ±infinity */
      return sign ? -INFINITY : INFINITY;
    }
    /* NaN */
    return NAN;
  }
  /* normalized */
  double m = 1.0 + (double)frac / 1024.0;
  double val = ldexp(m, (int)exp - 15);
  return sign ? -val : val;
}

static uint16_t _double_to_f16(double d) {
  if (isnan(d)) return 0x7E00; /* quiet NaN */
  int sign = d < 0.0 ? 1 : 0;
  if (sign) d = -d;
  if (isinf(d)) return (uint16_t)((sign << 15) | 0x7C00);
  if (d == 0.0) return (uint16_t)(sign << 15);

  /* normalized or subnormal */
  int e;
  double m = frexp(d, &e); /* m in [0.5, 1), d = m * 2^e */
  /* f16 exponent bias = 15, we need exp = e - 1 + 15 = e + 14 */
  int f16_exp = e + 14;

  if (f16_exp <= 0) {
    /* subnormal or zero */
    if (f16_exp < -10) return (uint16_t)(sign << 15); /* too small → zero */
    double subn = ldexp(m, f16_exp + 10); /* shift mantissa */
    uint16_t frac = (uint16_t)(subn + 0.5); /* round */
    if (frac > 0x3FF) frac = 0; /* overflow subnormal → zero */
    return (uint16_t)((sign << 15) | frac);
  }
  if (f16_exp >= 31) {
    /* overflow → infinity */
    return (uint16_t)((sign << 15) | 0x7C00);
  }
  /* normalized */
  double frac_d = (m - 0.5) * 2048.0; /* m in [0.5,1) → frac in [0,1024) */
  uint16_t frac = (uint16_t)(frac_d + 0.5);
  if (frac >= 0x400) { frac = 0; f16_exp++; }
  if (f16_exp >= 31) return (uint16_t)((sign << 15) | 0x7C00);
  return (uint16_t)((sign << 15) | (f16_exp << 10) | frac);
}

/* ---- Read float value as double ---- */

static double _float_read_f64(value_t v) {
  type_t t = value_get_type(v);
  switch (t->kind) {
    case TYPE_KIND_F16: return _f16_to_double(*(uint16_t *)value_get_data(v));
    case TYPE_KIND_F32: return (double)(*(float *)value_get_data(v));
    case TYPE_KIND_F64: return *(double *)value_get_data(v);
    default: return 0.0;
  }
}

/* ---- Write double to target float type ---- */

static void _float_write_f64(type_t t, void *data, double val) {
  switch (t->kind) {
    case TYPE_KIND_F16: *(uint16_t *)data = _double_to_f16(val); break;
    case TYPE_KIND_F32: *(float *)data    = (float)val; break;
    case TYPE_KIND_F64: *(double *)data   = val; break;
    default: break;
  }
}

static value_t _float_alloc_result(vm_t vm, type_t type, double val) {
  void *data = allocator_alloc(vm_get_allocator(vm), type->size);
  _float_write_f64(type, data, val);
  return _float_value_create(vm, type, data, true);
}

/* ---- Float type promotion ---- */

/**
 * @brief Pick the common type for binary float operation.
 * Larger size wins; same size → left operand's type.
 */
static type_t _float_common_type(type_t ta, type_t tb) {
  if (ta->size >= tb->size) return ta;
  return tb;
}

/**
 * @brief Coerce a float value to a target float type.
 * Used internally for type promotion. Not the same as safe_cast.
 */
static value_t _float_coerce(vm_t vm, value_t v, type_t to) {
  if (value_get_type(v) == to) return v;
  if (value_is_shadow(v))
    return vm_create_value_shadow(vm, to, NULL, true);
  double val = _float_read_f64(v);
  return _float_alloc_result(vm, to, val);
}

static type_t _float_promote(vm_t vm, value_t a, value_t b,
                             value_t *out_a, value_t *out_b) {
  type_t ta = value_get_type(a);
  type_t tb = value_get_type(b);
  if (!_is_float_kind(ta->kind) || !_is_float_kind(tb->kind))
    return NULL;
  type_t rt = _float_common_type(ta, tb);
  *out_a = _float_coerce(vm, a, rt);
  *out_b = _float_coerce(vm, b, rt);
  return rt;
}

/* ==================================================================
 * Per-type clone/dispose/equal/type_equal/type_extends/safe_cast/assignment
 * ================================================================== */

#define DEFINE_FLOAT_VTABLE(Prefix, ctype, KIND, SIZE, ALIGN, NAME)            \
                                                                               \
static value_t _##Prefix##_clone(allocator_t allocator, value_t self) {        \
  ctype *copy = (ctype *)allocator_alloc(allocator, SIZE);                     \
  memcpy(copy, value_get_data(self), SIZE);                                    \
  return value_create(allocator, value_get_type(self), copy, true);            \
}                                                                              \
                                                                               \
static void _##Prefix##_dispose(allocator_t allocator, value_t self) {         \
  void *d = value_get_data(self);                                              \
  allocator_free(allocator, &d);                                               \
}                                                                              \
                                                                               \
static value_t _##Prefix##_equal(vm_t vm, value_t a, value_t b) {             \
  type_t ta = value_get_type(a);                                               \
  type_t tb = value_get_type(b);                                               \
  if (!_is_float_kind(tb->kind))                                               \
    return create_error_value(vm, "cannot compare values of different kinds");  \
  if (value_is_shadow(a) || value_is_shadow(b)) {                              \
    type_t rt = _float_common_type(ta, tb);                                    \
    return vm_create_value_shadow(vm, rt, NULL, true);                         \
  }                                                                            \
  value_t pa, pb;                                                              \
  type_t rt = _float_promote(vm, a, b, &pa, &pb);                             \
  if (!rt) return create_error_value(vm, "float comparison failed");           \
  return create_bool_value(vm, _float_read_f64(pa) == _float_read_f64(pb));   \
}                                                                              \
                                                                               \
static value_t _##Prefix##_type_equal(vm_t vm, type_t a, type_t b) {          \
  (void)a;                                                                     \
  if (b->kind == TYPE_KIND_WILDCARD)                                           \
    return create_bool_value(vm, true);                                        \
  return create_bool_value(vm, b->kind == KIND);                              \
}                                                                              \
                                                                               \
static value_t _##Prefix##_type_extends(vm_t vm, type_t sub, type_t super) {  \
  (void)sub;                                                                   \
  if (super->kind == TYPE_KIND_WILDCARD)                                       \
    return create_bool_value(vm, true);                                        \
  return create_bool_value(vm, super->kind == KIND);                          \
}                                                                              \
                                                                               \
static value_t _##Prefix##_safe_cast(vm_t vm, value_t self, type_t to) {       \
  if (!_is_float_kind(to->kind))                                               \
    return create_error_value(vm,                                              \
        "cannot safe_cast %s to '%s'", NAME, to->name);                        \
  if (to == value_get_type(self)) return self;                                 \
  if (value_is_shadow(self))                                                   \
    return vm_create_value_shadow(vm, to, NULL, true);                         \
  return _float_coerce(vm, self, to);                                          \
}                                                                              \
                                                                               \
static value_t _const_##Prefix##_safe_cast(vm_t vm, value_t self, type_t to) {\
  if (!_is_float_kind(to->kind))                                               \
    return create_error_value(vm,                                              \
        "cannot safe_cast const %s to '%s'", NAME, to->name);                  \
  if (to->mut)                                                                 \
    return create_error_value(vm,                                              \
        "cannot safe_cast const %s to %s", NAME, NAME);                        \
  if (to == value_get_type(self)) return self;                                 \
  if (value_is_shadow(self))                                                   \
    return vm_create_value_shadow(vm, to, NULL, true);                         \
  return _float_coerce(vm, self, to);                                          \
}                                                                              \
                                                                               \
static value_t _##Prefix##_assignment(vm_t vm, value_t lvalue, value_t rvalue) {\
  type_t lt = value_get_type(lvalue);                                          \
  type_t rt = value_get_type(rvalue);                                          \
  if (!_is_float_kind(rt->kind))                                               \
    return create_error_value(vm,                                              \
        "cannot assign '%s' to '%s'", rt->name, lt->name);                     \
  if (value_is_shadow(lvalue) || value_is_shadow(rvalue)) {                    \
    value_set_initialized(lvalue, true);                                       \
    return create_void_value(vm);                                              \
  }                                                                            \
  double val = _float_read_f64(rvalue);                                       \
  _float_write_f64(lt, value_get_data(lvalue), val);                           \
  value_set_initialized(lvalue, true);                                         \
  return create_void_value(vm);                                                \
}                                                                              \
                                                                               \
static value_t _const_##Prefix##_clone(allocator_t allocator, value_t self) {  \
  ctype *copy = (ctype *)allocator_alloc(allocator, SIZE);                     \
  memcpy(copy, value_get_data(self), SIZE);                                    \
  return value_create(allocator, value_get_type(self), copy, true);            \
}

DEFINE_FLOAT_VTABLE(f16, uint16_t, TYPE_KIND_F16, 2, 2, "f16")
DEFINE_FLOAT_VTABLE(f32, float,    TYPE_KIND_F32, 4, 4, "f32")
DEFINE_FLOAT_VTABLE(f64, double,   TYPE_KIND_F64, 8, 8, "f64")

/* ==================================================================
 * Shared arithmetic/relational/unary implementations for all float types
 * ================================================================== */

static value_t _float_add(vm_t vm, value_t a, value_t b) {
  value_t pa, pb;
  type_t rt = _float_promote(vm, a, b, &pa, &pb);
  if (!rt) return create_error_value(vm, "operator +: incompatible float types");
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, rt, NULL, true);
  return _float_alloc_result(vm, rt, _float_read_f64(pa) + _float_read_f64(pb));
}

static value_t _float_sub(vm_t vm, value_t a, value_t b) {
  value_t pa, pb;
  type_t rt = _float_promote(vm, a, b, &pa, &pb);
  if (!rt) return create_error_value(vm, "operator -: incompatible float types");
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, rt, NULL, true);
  return _float_alloc_result(vm, rt, _float_read_f64(pa) - _float_read_f64(pb));
}

static value_t _float_mul(vm_t vm, value_t a, value_t b) {
  value_t pa, pb;
  type_t rt = _float_promote(vm, a, b, &pa, &pb);
  if (!rt) return create_error_value(vm, "operator *: incompatible float types");
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, rt, NULL, true);
  return _float_alloc_result(vm, rt, _float_read_f64(pa) * _float_read_f64(pb));
}

static value_t _float_div(vm_t vm, value_t a, value_t b) {
  value_t pa, pb;
  type_t rt = _float_promote(vm, a, b, &pa, &pb);
  if (!rt) return create_error_value(vm, "operator /: incompatible float types");
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, rt, NULL, true);
  if (_float_read_f64(pb) == 0.0)
    return create_error_value(vm, "division by zero");
  return _float_alloc_result(vm, rt, _float_read_f64(pa) / _float_read_f64(pb));
}

static value_t _float_mod(vm_t vm, value_t a, value_t b) {
  value_t pa, pb;
  type_t rt = _float_promote(vm, a, b, &pa, &pb);
  if (!rt) return create_error_value(vm, "operator %%: incompatible float types");
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, rt, NULL, true);
  if (_float_read_f64(pb) == 0.0)
    return create_error_value(vm, "division by zero");
  return _float_alloc_result(vm, rt, fmod(_float_read_f64(pa), _float_read_f64(pb)));
}

static value_t _float_band(vm_t vm, value_t a, value_t b) {
  (void)a; (void)b;
  return create_error_value(vm, "float does not support operator &");
}

static value_t _float_bor(vm_t vm, value_t a, value_t b) {
  (void)a; (void)b;
  return create_error_value(vm, "float does not support operator |");
}

static value_t _float_bxor(vm_t vm, value_t a, value_t b) {
  (void)a; (void)b;
  return create_error_value(vm, "float does not support operator ^");
}

static value_t _float_bnot(vm_t vm, value_t a) {
  (void)a;
  return create_error_value(vm, "float does not support operator ~");
}

static value_t _float_lnot(vm_t vm, value_t a) {
  if (value_is_shadow(a))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  return create_bool_value(vm, _float_read_f64(a) == 0.0);
}

static value_t _float_shl(vm_t vm, value_t a, value_t b) {
  (void)a; (void)b;
  return create_error_value(vm, "float does not support operator <<");
}

static value_t _float_shr(vm_t vm, value_t a, value_t b) {
  (void)a; (void)b;
  return create_error_value(vm, "float does not support operator >>");
}

static value_t _float_pos(vm_t vm, value_t a) {
  if (value_is_shadow(a))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  return _float_alloc_result(vm, value_get_type(a), +_float_read_f64(a));
}

static value_t _float_neg(vm_t vm, value_t a) {
  if (value_is_shadow(a))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  return _float_alloc_result(vm, value_get_type(a), -_float_read_f64(a));
}

static value_t _float_gt(vm_t vm, value_t a, value_t b) {
  value_t pa, pb;
  type_t rt = _float_promote(vm, a, b, &pa, &pb);
  if (!rt) return create_error_value(vm, "operator >: incompatible float types");
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, rt, NULL, true);
  return create_bool_value(vm, _float_read_f64(pa) > _float_read_f64(pb));
}

static value_t _float_lt(vm_t vm, value_t a, value_t b) {
  value_t pa, pb;
  type_t rt = _float_promote(vm, a, b, &pa, &pb);
  if (!rt) return create_error_value(vm, "operator <: incompatible float types");
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, rt, NULL, true);
  return create_bool_value(vm, _float_read_f64(pa) < _float_read_f64(pb));
}

/* ==================================================================
 * Static type singletons
 * ================================================================== */

#define DEFINE_FLOAT_TYPE(Prefix, ctype, KIND, SIZE, ALIGN, NAME)              \
type_t type_get_##Prefix##_type(allocator_t allocator) {                       \
  (void)allocator;                                                             \
  static struct _type_t t = {                                                  \
      .kind  = KIND,                                                           \
      .name  = (char *)NAME,                                                   \
      .size  = SIZE,                                                           \
      .align = ALIGN,                                                          \
      .mut   = true,                                                           \
      .vtable = {                                                              \
          .clone        = _##Prefix##_clone,                                   \
          .dispose      = _##Prefix##_dispose,                                 \
          .equal        = _##Prefix##_equal,                                   \
          .extends      = NULL,                                                \
          .type_equal   = _##Prefix##_type_equal,                              \
          .type_extends = _##Prefix##_type_extends,                            \
          .band         = _float_band,                                         \
          .bor          = _float_bor,                                          \
          .bxor         = _float_bxor,                                         \
          .bnot         = _float_bnot,                                         \
          .lnot         = _float_lnot,                                         \
          .add          = _float_add,                                          \
          .sub          = _float_sub,                                          \
          .mul          = _float_mul,                                          \
          .div          = _float_div,                                          \
          .mod          = _float_mod,                                          \
          .shl          = _float_shl,                                          \
          .shr          = _float_shr,                                          \
          .pos          = _float_pos,                                          \
          .neg          = _float_neg,                                          \
          .gt           = _float_gt,                                           \
          .lt           = _float_lt,                                           \
          .safe_cast    = _##Prefix##_safe_cast,                               \
          .assignment   = _##Prefix##_assignment,                              \
          .to_string    = NULL,                                                \
      },                                                                       \
  };                                                                           \
  return &t;                                                                   \
}                                                                              \
                                                                               \
type_t type_get_const_##Prefix##_type(allocator_t allocator) {                 \
  (void)allocator;                                                             \
  static struct _type_t t = {                                                  \
      .kind  = KIND,                                                           \
      .name  = (char *)"const " NAME,                                          \
      .size  = SIZE,                                                           \
      .align = ALIGN,                                                          \
      .mut   = false,                                                          \
      .vtable = {                                                              \
          .clone        = _const_##Prefix##_clone,                             \
          .dispose      = _##Prefix##_dispose,                                 \
          .equal        = _##Prefix##_equal,                                   \
          .extends      = NULL,                                                \
          .type_equal   = _##Prefix##_type_equal,                              \
          .type_extends = _##Prefix##_type_extends,                            \
          .band         = _float_band,                                         \
          .bor          = _float_bor,                                          \
          .bxor         = _float_bxor,                                         \
          .bnot         = _float_bnot,                                         \
          .lnot         = _float_lnot,                                         \
          .add          = _float_add,                                          \
          .sub          = _float_sub,                                          \
          .mul          = _float_mul,                                          \
          .div          = _float_div,                                          \
          .mod          = _float_mod,                                          \
          .shl          = _float_shl,                                          \
          .shr          = _float_shr,                                          \
          .pos          = _float_pos,                                          \
          .neg          = _float_neg,                                          \
          .gt           = _float_gt,                                           \
          .lt           = _float_lt,                                           \
          .safe_cast    = _const_##Prefix##_safe_cast,                         \
          .assignment   = NULL,                                                \
          .to_string    = NULL,                                                \
      },                                                                       \
  };                                                                           \
  return &t;                                                                   \
}                                                                              \
                                                                               \
value_t create_##Prefix##_value(vm_t vm, ctype val) {                          \
  allocator_t alloc = vm_get_allocator(vm);                                    \
  ctype *data = (ctype *)allocator_alloc(alloc, SIZE);                         \
  *data = val;                                                                 \
  type_t t = (type_t)value_get_data(vm_get_##Prefix##_type(vm));               \
  value_t v = value_create(alloc, t, data, true);                              \
  scope_t scope = vm_get_current_scope(vm);                                    \
  if (scope) { vec_push(scope->values, v); }                                   \
  return v;                                                                    \
}

DEFINE_FLOAT_TYPE(f16, uint16_t, TYPE_KIND_F16, 2, 2, "f16")
DEFINE_FLOAT_TYPE(f32, float,    TYPE_KIND_F32, 4, 4, "f32")
DEFINE_FLOAT_TYPE(f64, double,   TYPE_KIND_F64, 8, 8, "f64")
