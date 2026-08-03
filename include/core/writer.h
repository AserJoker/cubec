#ifndef _H_CUBEC_CORE_WRITER_
#define _H_CUBEC_CORE_WRITER_
#include "core/string.h"
#include "core/type.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
extern type_t g_writer_type;
struct _writer_t;
typedef struct _writer_t *writer_t;
void writer_append(writer_t self, const char *line);
void writer_newline(writer_t self, int32_t indent);
void writer_dedent_current_line(writer_t self, int32_t delta);
string_t writer_get_current_line(writer_t self);
string_t writer_get_string(writer_t self);

#ifdef __cplusplus
}
#endif
#endif