#ifndef _H_CUBEC_C_IR_ENUM_
#define _H_CUBEC_C_IR_ENUM_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enum item (name = value).
 */
typedef struct _c_ir_enum_item_t {
  string_t name;    /**< Mangled name (e.g., "m3a7_Color_Red") */
  string_t value;   /**< Value string (e.g., "0", "1") */
} *c_ir_enum_item_t;

c_ir_enum_item_t c_ir_enum_item_create(allocator_t allocator,
                                         const char *name, const char *value);
void c_ir_enum_item_dispose(allocator_t allocator, c_ir_enum_item_t *item);

/**
 * @brief Enum definition.
 */
typedef struct _c_ir_enum_def_t *c_ir_enum_def_t;

struct _c_ir_enum_def_t {
  enum c_ir_kind kind;
  location_t source_loc;
  string_t name;           /**< Mangled type name (e.g., "m3a7_Color") */
  c_type_t backing_type;   /**< Underlying type, or NULL */
  vec_t items;             /**< c_ir_enum_item_t */
};

c_ir_enum_def_t c_ir_enum_def_create(allocator_t allocator, const char *name,
                                       c_type_t backing_type, vec_t items,
                                       location_t source_loc);
void c_ir_enum_def_dispose(allocator_t allocator, c_ir_enum_def_t *node);

#ifdef __cplusplus
}
#endif
#endif
