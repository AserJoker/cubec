#ifndef _H_CUBEC_ENGINE_VALUE_
#define _H_CUBEC_ENGINE_VALUE_
#include "core/allocator.h"
#include "core/class.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Forward declaration — full definition in engine/stype.h.
 */
struct stype_t;
typedef struct stype_t stype_t;

/**
 * @brief Uniform object model — all runtime values are value_t.
 *
 * type  → stype_t pointer (vtable + metadata), ref (not owned)
 * data  → raw data buffer (NULL in shadow mode)
 * own   → whether this value owns (frees) data on dispose
 */
typedef struct value_t {
  stype_t *type;   /**< Type object, ref (never NULL for live values) */
  void    *data;   /**< Raw data buffer, or NULL for shadow */
  bool     own;    /**< True = value owns data, false = borrowed ref */
} value_t;

/** @brief Initialization parameters for value_t. */
typedef struct value_init_t {
  stype_t *type;
  void    *data;
  bool     own;
} value_init_t;

/** @brief Type descriptor for allocator_create. */
extern class_t g_value_class;

/**
 * @brief Create a value with given type, data, and ownership.
 */
value_t *value_create(allocator_t allocator, stype_t *type, void *data,
                      bool own);

/**
 * @brief Dispose a value. If own=true, calls type->vtable.dispose(data).
 */
void value_dispose(value_t *self, allocator_t allocator);

/* ---- Accessors ---- */

stype_t *value_get_type(const value_t *self);
void    *value_get_data(const value_t *self);
bool     value_is_own(const value_t *self);
bool     value_is_shadow(const value_t *self);

#ifdef __cplusplus
}
#endif
#endif
