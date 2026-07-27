#ifndef _H_CUBEC_WRITER_
#define _H_CUBEC_WRITER_
#include "c/c_ir.h"
#include "c/c_ir_unit.h"
#include "core/allocator.h"
#include "core/string.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Write a C IR compilation unit to .c and .h file contents.
 *
 * @param unit   The C IR compilation unit
 * @param out_h  Output string for .h file content (caller must initialize)
 * @param out_c  Output string for .c file content (caller must initialize)
 */
void writer_write_unit(allocator_t allocator, c_ir_unit_t unit,
                         string_t out_h, string_t out_c);

#ifdef __cplusplus
}
#endif
#endif
