#ifndef _H_CUBEC_BUILDER_CUBEC_WRITER_
#define _H_CUBEC_BUILDER_CUBEC_WRITER_

#include "core/string.h"
#include "cubec/program.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generate a cubec interface file from a program AST.
 *
 * Traverses the AST and emits extern declarations for all exported symbols
 * (functions, variables, types) into the output string. The result is a
 * valid .cubec source file that can be imported by other modules.
 *
 * @param allocator  Allocator for string operations
 * @param program    The program AST (after checking)
 * @param out        Output string to append the interface to
 */
void cubec_write_interface(allocator_t allocator, node_t program, string_t out);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_BUILDER_CUBEC_WRITER_ */
