#ifndef _H_C_TYPE_
#define _H_C_TYPE_
#include "c/writer.h"
#ifdef __cplusplus
extern "C" {
#endif
void c_type_declarator(c_writer_t writer, type_t type);
void c_type_declaration(c_writer_t writer, type_t type);
void c_type(c_writer_t writer, type_t type);
#ifdef __cplusplus
}
#endif
#endif