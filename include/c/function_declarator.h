#ifndef _H_C_FUNCTION_DECLARATION_
#define _H_C_FUNCTION_DECLARATION_
#include "core/stream.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_c_function_declarator(context_t ctx, value_t value, stream_t stream);
void write_c_function_declaration(context_t ctx, value_t value,
                                  stream_t stream);
#ifdef __cplusplus
}
#endif
#endif