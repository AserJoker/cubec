#ifndef _H_CUBEC_ENGINE_FUNCTION_
#define _H_CUBEC_ENGINE_FUNCTION_

#include "core/allocator.h"
#include "core/node.h"
#include "core/vec.h"
#include "engine/def.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function template — represents a function declaration.
 *
 * Two-layer design: function_t is the template (with generic params),
 * implements holds function_instance_t objects with concrete arguments,
 * return type, body, and captures.
 *
 * Non-generic functions have params=NULL and a single hash=0 instance.
 *
 * Owned by scope_t. Borrowed by name_t.ref.
 */
struct _function_t {
  def_t header;
  bool is_export;
  bool is_exportlib;
  bool is_inline;
  bool is_extern;
  bool is_builtin;
  bool is_comptime;
  bool is_c_variadic;
  vec_t params;     /* generic params (nullable) */
  vec_t implements; /* function_instance_t array (nullable) */
};

typedef struct _function_t *function_t;

/**
 * @brief Create a function_t with modifiers from the AST.
 * @param allocator     Allocator for this object
 * @param node          AST function declaration node (borrowing reference)
 * @param is_export     Exported from module
 * @param is_exportlib  Exported with C ABI
 * @param is_inline     Inline function
 * @param is_extern     External linkage (no body)
 * @param is_builtin    Compiler-provided (no body)
 * @param is_comptime   Compile-time evaluated
 * @param is_c_variadic C-style variadic
 * @return New function_t with params=NULL, implements=NULL
 */
function_t function_create(allocator_t allocator, node_t node,
                           bool is_export, bool is_exportlib, bool is_inline,
                           bool is_extern, bool is_builtin, bool is_comptime,
                           bool is_c_variadic);

/** @brief Dispose a function_t and its owned sub-objects. */
void function_dispose(function_t func);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_ENGINE_FUNCTION_ */
