#ifndef _H_CUBC_CORE_POSITION_
#define _H_CUBC_CORE_POSITION_
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
typedef struct _position_t {
  const char *offset;
  size_t column;
  size_t line;
} position_t;
#ifdef __cplusplus
}
#endif
#endif