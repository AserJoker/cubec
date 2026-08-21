#ifndef _H_CUBEC_ENGINE_GENERIC_INFERENCE_
#define _H_CUBEC_ENGINE_GENERIC_INFERENCE_
#include "engine/type.h"
#include "engine/value.h"
#include "engine/generic_fn_type.h"
#include "engine/generic_param.h"
#include "core/allocator.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque inference context — hides infer_entry_t from vtable consumers.
 *
 * Created internally by generic_fn_call_with_inference, passed to
 * vtable.infer_walk as void* ctx. Type implementations should only
 * interact with it via the infer_walk_* helper functions.
 */
typedef void *infer_ctx_t;

/* ---- Helper functions for vtable.infer_walk implementations ---- */

/**
 * @brief Recurse into a sub-type pair during inference.
 *
 * Called by type-specific infer_walk implementations to match nested types.
 * This dispatches through the formal type's vtable.infer_walk entry.
 *
 * @param vm     VM context
 * @param ctx    Opaque inference context
 * @param formal Formal sub-type (may contain placeholders)
 * @param actual Actual sub-type from the call argument
 * @return true on match/inference, false on mismatch
 */
bool infer_walk_recurse(vm_t vm, infer_ctx_t ctx, type_t formal, type_t actual);

/**
 * @brief Assign a type parameter by name during inference.
 *
 * Called when a GENERIC_PARAM placeholder is encountered in a type position.
 * If the param was already inferred, checks for exact type match (no safe_cast).
 *
 * @param vm       VM context
 * @param ctx      Opaque inference context
 * @param name     The placeholder/param name (e.g. "T")
 * @param actual   The concrete type to assign
 * @return true on successful assignment or match, false on mismatch
 */
bool infer_walk_assign_param(vm_t vm, infer_ctx_t ctx,
                             const char *name, type_t actual);

/**
 * @brief Assign a value parameter during inference (e.g. array count).
 *
 * Called when a placeholder value (from a value-type generic param like N:u64)
 * is encountered in a value position within a type (e.g. array count).
 * Finds the matching uninferred value param by declared type and assigns
 * the actual value.
 *
 * @param vm         VM context
 * @param ctx        Opaque inference context
 * @param value_type The declared type of the value param (e.g. u64)
 * @param data       The concrete value data (integer bytes)
 * @return true on successful assignment, false on mismatch/no match
 */
bool infer_walk_assign_value(vm_t vm, infer_ctx_t ctx,
                             type_t value_type, void *data);

/* ---- Main entry point ---- */

/**
 * @brief Generic function call with type inference.
 *
 * Implements the inference pipeline:
 *   1. Initialize placeholders for each generic param
 *   2. Assign any explicit generic args (from subscript syntax)
 *   3. Infer remaining params by matching formal types against actual arg types
 *   4. Validate extends constraints
 *   5. Instantiate (value_instantiate) and call (value_call)
 *
 * @param vm          VM context
 * @param generic_val the generic function value (TYPE_KIND_GENERIC_FN)
 * @param type_argc   number of explicit type arguments (0 = all inferred)
 * @param type_argv   explicit type argument values (may be NULL)
 * @param call_argc   number of call arguments
 * @param call_argv   call argument values
 * @return the call result, or an exception on failure
 */
value_t generic_fn_call_with_inference(vm_t vm, value_t generic_val,
                                       size_t type_argc, value_t *type_argv,
                                       size_t call_argc, value_t *call_argv);

/**
 * @brief Create a placeholder type for a generic type parameter.
 *
 * TYPE_KIND_GENERIC_PARAM: a temporary type used during inference.
 * name is the generic parameter name (e.g. "T"), cloned (owned).
 * This placeholder participates in temporary type construction (e.g. []T)
 * without error, and records the name for later assignment to scope.
 */
type_t generic_param_placeholder_create(allocator_t allocator, const char *name);

/**
 * @brief Create a placeholder type for a generic pack parameter.
 *
 * TYPE_KIND_GENERIC_PACK: a temporary type for pack inference.
 * name is the pack parameter name (e.g. "T"), cloned (owned).
 * At the same position, absorbs all subsequent types.
 */
type_t generic_pack_placeholder_create(allocator_t allocator, const char *name);

/**
 * @brief Check if a type is a generic param placeholder.
 */
bool type_is_generic_param_placeholder(type_t t);

/**
 * @brief Check if a type is a generic pack placeholder.
 */
bool type_is_generic_pack_placeholder(type_t t);

/**
 * @brief Get the name from a placeholder type (borrowed).
 */
const char *generic_placeholder_get_name(type_t t);

/* ---- Per-type infer_walk implementations ---- */
/* These are referenced by type-specific _make_*_vtable() factory functions. */

extern bool _pointer_infer_walk(vm_t vm, type_t formal, type_t actual, void *ctx);
extern bool _array_infer_walk(vm_t vm, type_t formal, type_t actual, void *ctx);
extern bool _slice_infer_walk(vm_t vm, type_t formal, type_t actual, void *ctx);
extern bool _tuple_infer_walk(vm_t vm, type_t formal, type_t actual, void *ctx);
extern bool _callable_infer_walk(vm_t vm, type_t formal, type_t actual, void *ctx);
extern bool _struct_infer_walk(vm_t vm, type_t formal, type_t actual, void *ctx);
extern bool _union_infer_walk(vm_t vm, type_t formal, type_t actual, void *ctx);
extern bool _cunion_infer_walk(vm_t vm, type_t formal, type_t actual, void *ctx);
extern bool _enum_infer_walk(vm_t vm, type_t formal, type_t actual, void *ctx);

#ifdef __cplusplus
}
#endif
#endif
