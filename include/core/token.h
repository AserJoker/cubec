#ifndef _H_CUBEC_CORE_TOKEN_
#define _H_CUBEC_CORE_TOKEN_
#include "core/location.h"
#include "core/type.h"
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
struct _token_t;
typedef struct _token_t *token_t;
extern type_t g_token_type;
struct _token_init_t {
  uint32_t kind;
  location_t location;
};
typedef struct _token_init_t token_init_t;
uint32_t token_get_kind(token_t self);
location_t *token_get_location(token_t self);
bool token_is(token_t self, uint32_t kind, const char *text);
#ifdef __cplusplus
}
#endif
#endif