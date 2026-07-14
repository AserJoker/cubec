#ifndef _H_CUBEC_ENGINE_TYPE_LAYOUT_
#define _H_CUBEC_ENGINE_TYPE_LAYOUT_
#include "engine/semantic_type.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute the layout (size, alignment, field offsets) for a type.
 *        For struct/union/cunion, computes field offsets and total size.
 *        For primitives, sets canonical sizes.
 *        For pointer/slice/array, computes based on target platform.
 *        Marks the type as complete (is_incomplete = false) after layout.
 *
 * @param type   The semantic type to compute layout for.
 * @param ptr_size  Pointer size for the target platform (4 or 8).
 */
void type_layout_compute(semantic_type_t type, size_t ptr_size);

#ifdef __cplusplus
}
#endif
#endif
