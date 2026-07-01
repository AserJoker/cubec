#ifndef _H_CUBEC_CUBEC_TOKEN_
#define _H_CUBEC_CUBEC_TOKEN_
#include "core/allocator.h"
#include "core/position.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "core/token.h"
#include <stddef.h>
#include <stdint.h>
enum _cubec_token_kind_t {
  CUBEC_TOKEN_WHITESPACE,
  CUBEC_TOKEN_EOF,
  CUBEC_TOKEN_COMMENT,
  CUBEC_TOKEN_MULTILINE_COMMENT,
  CUBEC_TOKEN_IDENTIFIER,
  CUBEC_TOKEN_NUMERIC,
  CUBEC_TOKEN_SYMBOL,
  CUBEC_TOKEN_KEYWORD,
  CUBEC_TOKEN_STRING,
};
typedef enum _cubec_token_kind_t cubec_token_kind_t;
token_t read_token(allocator_t allocator, position_t *position,
                   const char *filename);
vec_t resolve_token_list(allocator_t allocator, const char *filename,
                         const char *source);
#ifdef __cplusplus
}
#endif
#endif