#ifndef _H_CUBEC_CORE_ENV_
#define _H_CUBEC_CORE_ENV_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Resolved environment configuration — all strings are owned by the env module */
typedef struct {
  const char *cubec_home;   /* CUBEC_HOME or auto-detected */
  const char *cc;           /* C compiler path (CC env or PATH search) */
  const char *ar;           /* Archiver path (AR env or PATH search) */
  const char *cflags;       /* CFLAGS or "" */
  const char *ldflags;      /* LDFLAGS or "" */
} env_config_t;

/* Initialize env module — discovers and caches configuration.
   Call once at startup. Returns 0 on success. */
int env_init(void);

/* Get the resolved configuration (valid after env_init). */
const env_config_t *env_get(void);

/* Clean up env module resources. */
void env_dispose(void);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_CORE_ENV_ */
