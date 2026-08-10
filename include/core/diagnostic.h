#ifndef _H_CUBEC_CORE_DIAGNOSTIC_
#define _H_CUBEC_CORE_DIAGNOSTIC_
#include "core/location.h"
#include "core/class.h"
#include "core/vec.h"
#include <stddef.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

enum diagnostic_severity {
  DIAGNOSTIC_ERROR,
  DIAGNOSTIC_WARNING,
  DIAGNOSTIC_NOTE
};

struct diagnostic_note {
  location_t location;
  char message[256];
};

struct diagnostic {
  enum diagnostic_severity severity;
  location_t primary;
  char message[512];
  vec_t notes;
};

struct _diagnostic_list_t;
typedef struct _diagnostic_list_t *diagnostic_list_t;

typedef struct _diagnostic_list_init_t diagnostic_list_init_t;
struct _diagnostic_list_init_t {
  FILE *output;
};

extern class_t g_diagnostic_list_class;

size_t diagnostic_list_get_size(diagnostic_list_t self);
struct diagnostic *diagnostic_list_get(diagnostic_list_t self, size_t idx);
size_t diagnostic_list_get_error_count(diagnostic_list_t self);

struct diagnostic *diagnostic_list_push(diagnostic_list_t self,
                                        enum diagnostic_severity severity,
                                        location_t primary, const char *fmt,
                                        ...);

void diagnostic_list_push_note(diagnostic_list_t self, location_t location,
                               const char *fmt, ...);

struct context;
void diagnostic_list_emit(diagnostic_list_t self, struct context *ctx);

void diagnostic_list_clear(diagnostic_list_t self);

#ifdef __cplusplus
}
#endif
#endif
