#ifndef _H_CUBEC_C_LOWER_
#define _H_CUBEC_C_LOWER_
#include "c/c_ir.h"
#include "c/c_ir_unit.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Lower a cubec program to C IR.
 *
 * Translates the checked AST + semantic info into a C IR compilation unit.
 * The caller must dispose the result with c_ir_dispose().
 *
 * @param allocator           Allocator for C IR nodes
 * @param ctx                 Checker context (with resolved symbols and types)
 * @param program             Root AST node (CUBEC_NODE_PROGRAM)
 * @param generate_executable If true, generate a C main() wrapper that calls
 *                            the cubec exported main function
 * @return                    C IR compilation unit, or NULL on error
 */
c_ir_unit_t lower_program(allocator_t allocator, context_t ctx, node_t program,
                           bool generate_executable);

#ifdef __cplusplus
}
#endif
#endif
