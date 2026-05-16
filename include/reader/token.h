#ifndef _H_READER_TOKEN_
#define _H_READER_TOKEN_
#include "core/allocator.h"
#include "core/array.h"
#include "core/location.h"
#include "core/position.h"
#include "reader/token_type.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _token_t *token_t;
struct _token_t {
  token_type_t type;
  location_t loc;
};
typedef struct _token_stream_t *token_stream_t;
struct _token_stream_t {
  const char *filename;
  array_t tokens;
  size_t position;
};
token_t read_space_token(allocator_t allocator, position_t *position,
                         const char *end, const char *filename);
token_t read_comment_token(allocator_t allocator, position_t *position,
                           const char *end, const char *filename);
token_t read_numeric_token(allocator_t allocator, position_t *position,
                           const char *end, const char *filename);
token_t read_char_token(allocator_t allocator, position_t *position,
                        const char *end, const char *filename);
token_t read_identifier_token(allocator_t allocator, position_t *position,
                              const char *end, const char *filename);
token_t read_symbol_token(allocator_t allocator, position_t *position,
                          const char *end, const char *filename);
token_t read_token(allocator_t allocator, position_t *position, const char *end,
                   const char *filename);
token_stream_t create_token_stream(allocator_t allocator, position_t *position,
                                   const char *end, const char *filename);
token_t token_stream_get(token_stream_t stream);
bool token_is(token_t token, token_type_t type, const char *src);
void skip_comments(token_stream_t stream);

#ifdef __cplusplus
}
#endif
#endif