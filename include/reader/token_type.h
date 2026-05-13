#ifndef _H_READER_TOKEN_TYPE_
#define _H_READER_TOKEN_TYPE_
#ifdef __cplusplus
extern "C" {
#endif
typedef enum _token_type_t {
  TOKEN_TYPE_SPACE,
  TOKEN_TYPE_COMMENT,
  TOKEN_TYPE_NUMERIC,
  TOKEN_TYPE_IDENTIFIER,
  TOKEN_TYPE_KEYWORD,
  TOKEN_TYPE_STRING,
  TOKEN_TYPE_CHARACTOR,
  TOKEN_TYPE_SYMBOL,
} token_type_t;
#ifdef __cplusplus
}
#endif
#endif