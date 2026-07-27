#ifndef _H_CUBEC_C_MANGLE_
#define _H_CUBEC_C_MANGLE_
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute module hash from a file path using FNV-1a.
 * @param path Module file path (e.g., "src/foo.cubec")
 * @return 4-character hex string (e.g., "m3a7"), allocated from allocator.
 */
string_t mangle_module_hash(allocator_t allocator, const char *path);

/** @brief Mangle a global name: module_hash + "_" + name. */
string_t mangle_name(allocator_t allocator, const char *module_hash,
                      const char *name);

/** @brief Mangle a method: module_hash + "_" + type_name + "_" + method_name. */
string_t mangle_method(allocator_t allocator, const char *module_hash,
                        const char *type_name, const char *method_name);

/** @brief Mangle a generic instantiation: module_hash + "_" + name + "_" + type_args joined. */
string_t mangle_generic(allocator_t allocator, const char *module_hash,
                         const char *name, vec_t type_arg_names);

/** @brief Mangle an enum item: module_hash + "_" + type_name + "_" + item_name. */
string_t mangle_enum_item(allocator_t allocator, const char *module_hash,
                           const char *type_name, const char *item_name);

/** @brief Mangle a static field: module_hash + "_" + type_name + "_" + field_name. */
string_t mangle_static_field(allocator_t allocator, const char *module_hash,
                              const char *type_name, const char *field_name);

/** @brief Mangle a nested function: module_hash + "_" + parent_name + "__" + index. */
string_t mangle_nested_func(allocator_t allocator, const char *module_hash,
                             const char *parent_name, int index);

#ifdef __cplusplus
}
#endif
#endif
