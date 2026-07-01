#ifndef _H_CUBEC_CORE_POSITION_
#define _H_CUBEC_CORE_POSITION_
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <stdint.h>
typedef struct _position_t position_t;
struct _position_t {
  size_t line;
  size_t column;
  const char *offset;
};
#ifdef __cplusplus
}
#endif
#endif