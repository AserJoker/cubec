#ifndef _H_CUBEC_ENGINE_VALUE_
#define _H_CUBEC_ENGINE_VALUE_

#include "core/allocator.h"
#include "core/node.h"
#include "engine/def.h"
#include "engine/comptime_value.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _stype_t;
typedef struct _stype_t *stype_t;

/**
 * @brief Value binding — represents a variable/constant declaration.
 *
 * Owned by scope_t. Borrowed by name_t.ref.
 * stype and data are NULL after phase 1; filled during phase 2.
 */
struct _value_t {
  def_t header;
  stype_t stype;              /* value's type (NULL in phase 1) */
  comptime_value_t data;      /* comptime eval result, borrowing (NULL in phase 1) */
  bool is_export;
  bool is_exportlib;
  bool is_extern;
  bool is_builtin;
  bool is_comptime;
  bool is_using;
};

typedef struct _value_t *value_t;

/**
 * @brief Create a value_t with modifiers from the AST.
 * @param allocator    Allocator for this object
 * @param node         AST declaration node (borrowing reference)
 * @param is_export    Exported from module
 * @param is_exportlib Exported with C ABI
 * @param is_extern    External linkage (no initializer)
 * @param is_builtin   Compiler-provided (no initializer)
 * @param is_comptime  Compile-time evaluated
 * @param is_using     Auto-defer at scope exit
 * @return New value_t with stype=NULL, data=NULL
 */
value_t value_create(allocator_t allocator, node_t node,
                     bool is_export, bool is_exportlib, bool is_extern,
                     bool is_builtin, bool is_comptime, bool is_using);

/** @brief Dispose a value_t. */
void value_dispose(value_t value);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_ENGINE_VALUE_ */
