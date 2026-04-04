#ifndef _H_CUBEC_WRITE_CONTEXT_
#define _H_CUBEC_WRITE_CONTEXT_
#include "core/allocator.h"
#ifdef __cplusplus__
extern "C" {
#endif
struct _cubec_write_context {
  size_t indent;
  cubec_allocator_t allocator;
};
typedef struct _cubec_write_context cubec_write_context;
#ifdef __cplusplus__
}
#endif
#endif