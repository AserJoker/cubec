#ifndef _H_CUBEC_WRITER_CUBEC_AST_WRITER_
#define _H_CUBEC_WRITER_CUBEC_AST_WRITER_

#include "core/string.h"
#include "cubec/program.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Serialize the entire AST back to valid Cubec source code.
 *
 * Traverses the program AST recursively and writes valid Cubec syntax
 * to the output string. Supports round-trip testing and debugging.
 *
 * @param allocator  Allocator for string operations
 * @param program    The program AST node (must be CUBEC_NODE_PROGRAM)
 * @param out        Output string to append the source to
 */
void cubec_ast_write(allocator_t allocator, node_t program, string_t out);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_WRITER_CUBEC_AST_WRITER_ */
