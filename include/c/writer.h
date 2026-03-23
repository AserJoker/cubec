#ifndef _H_CUBEC_C_WRITER_
#define _H_CUBEC_C_WRITER_
#include "ast/node.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef cubec_value_t (*cubec_writer_fn_t)(cubec_context_t self,
                                           cubec_ast_node_t node,
                                           const char *filename,
                                           cubec_string_t *output);
cubec_value_t cubec_c_write(cubec_context_t self, cubec_ast_node_t node,
                            const char *filename, cubec_string_t *output);
cubec_value_t cubec_c_create_error(cubec_context_t self, cubec_ast_node_t node,
                                   const char *filename, const char *fmt, ...);
#ifdef __cplusplus
}
#endif
#endif