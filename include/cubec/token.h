#ifndef _H_CUBEC_CUBEC_TOKEN_
#define _H_CUBEC_CUBEC_TOKEN_
#include "core/allocator.h"
#include "core/position.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "core/token.h"
#include "engine/context.h"
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
  CUBEC_TOKEN_CHAR
};
typedef enum _cubec_token_kind_t cubec_token_kind_t;
token_t read_token(context_t ctx, position_t *position,
                   const char *filename);
vec_t resolve_token_list(context_t ctx, const char *filename,
                         const char *source);
#ifdef __cplusplus
}
#endif
#endif