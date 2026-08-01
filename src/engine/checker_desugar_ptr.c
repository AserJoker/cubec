/**
 * @file checker_desugar_ptr.c
 * @brief Pass 8: Pointer Arithmetic — slice subscript → ptr + offset + deref.
 *
 * Strategy:
 *   - After Pass 3, slices are structs { ptr; start; length; }.
 *   - Slice subscript is no longer a special operation — it's just
 *     member access + pointer dereference on the degraded struct.
 *   - The C backend handles subscript on pointer-typed members natively.
 *   - This pass is a no-op: all pointer arithmetic is implicit in the
 *     degraded AST structure.
 */
#include "engine/checker_desugar_util.h"

void desugar_pass8_ptr_arith(context_t ctx, vec_t statements) {
  (void)ctx;
  (void)statements;
  /* The C backend generates pointer arithmetic from the degraded types
   * created by Pass 3. No AST transformation needed here. */
}
