#ifndef _H_CORE_STREAM_
#define _H_CORE_STREAM_
#include "core/allocator.h"
#include "core/location.h"
#include "core/string.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _stream_t *stream_t;
stream_t create_stream(allocator_t allocator);
void stream_write(stream_t stream, const char *fmt, ...);
void stream_write_location(stream_t stream, location_t loc);
void stream_newline(stream_t stream);
void stream_popline(stream_t stream);
void stream_inc_indent(stream_t stream);
void stream_dec_indent(stream_t stream);
void stream_set_indent(stream_t stream, size_t indent);
size_t stream_get_indent(stream_t stream);
void stream_set_base_indent(stream_t stream, size_t base_indent);
size_t stream_get_base_indent(stream_t stream);
string_t stream_get_string(stream_t stream);
void stream_merge(stream_t stream, stream_t another);
#ifdef __cplusplus
}
#endif
#endif