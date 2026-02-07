#ifndef _H_CUBEC_ASTWRITER_PROGRAM_
#define _H_CUBEC_ASTWRITER_PROGRAM_
#include "ast/program.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_write_ast_program(cubec_allocator_t allocator,
                                      cubec_ast_program_t program);
#ifdef __cplusplus
}
#endif
#endif