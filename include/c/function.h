#ifndef _H_C_FUNCTION_
#define _H_C_FUNCTION_
#include "c/writer.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
void c_function_declar(c_writer_t writer, value_t func);
void c_function_declaration(c_writer_t writer, value_t func);
#ifdef __cplusplus
}
#endif
#endif