#include "core/env.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#define ENV_STRDUP _strdup
#else
#include <unistd.h>
#define ENV_STRDUP strdup
#endif

static env_config_t g_env = {NULL, NULL, NULL, NULL, NULL};

/* Search PATH for an executable, returns malloc'd path or NULL */
static char *find_in_path(const char *name) {
#ifdef _WIN32
  char buf[MAX_PATH];
  if (SearchPathA(NULL, name, ".exe", MAX_PATH, buf, NULL)) {
    return ENV_STRDUP(buf);
  }
  if (SearchPathA(NULL, name, NULL, MAX_PATH, buf, NULL)) {
    return ENV_STRDUP(buf);
  }
  return NULL;
#else
  const char *path_env = getenv("PATH");
  if (!path_env) return NULL;

  char *paths = ENV_STRDUP(path_env);
  if (!paths) return NULL;

  char *saveptr = NULL;
  char *dir = strtok_r(paths, ":", &saveptr);
  while (dir) {
    size_t dirlen = strlen(dir);
    size_t namelen = strlen(name);
    char *candidate = (char *)malloc(dirlen + 1 + namelen + 1);
    if (!candidate) { free(paths); return NULL; }
    memcpy(candidate, dir, dirlen);
    candidate[dirlen] = '/';
    memcpy(candidate + dirlen + 1, name, namelen + 1);

    if (access(candidate, X_OK) == 0) {
      free(paths);
      return candidate;
    }
    free(candidate);
    dir = strtok_r(NULL, ":", &saveptr);
  }
  free(paths);
  return NULL;
#endif
}

/* Discover C compiler: CC env var, then PATH search for cc, gcc, clang */
static char *discover_cc(void) {
  const char *cc_env = getenv("CC");
  if (cc_env && cc_env[0] != '\0') {
    return ENV_STRDUP(cc_env);
  }

  static const char *candidates[] = {"cc", "gcc", "clang", NULL};
  for (int i = 0; candidates[i]; i++) {
    char *found = find_in_path(candidates[i]);
    if (found) return found;
  }
  return NULL;
}

/* Discover archiver: AR env var, then PATH search for llvm-ar, ar */
static char *discover_ar(void) {
  const char *ar_env = getenv("AR");
  if (ar_env && ar_env[0] != '\0') {
    return ENV_STRDUP(ar_env);
  }

  static const char *candidates[] = {"llvm-ar", "ar", NULL};
  for (int i = 0; candidates[i]; i++) {
    char *found = find_in_path(candidates[i]);
    if (found) return found;
  }
  return NULL;
}

/* Discover CUBEC_HOME: env var, then platform-default locations */
static char *discover_cubec_home(void) {
  const char *home_env = getenv("CUBEC_HOME");
  if (home_env && home_env[0] != '\0') {
    return ENV_STRDUP(home_env);
  }

#ifdef _WIN32
  const char *appdata = getenv("APPDATA");
  if (appdata && appdata[0] != '\0') {
    size_t len = strlen(appdata);
    char *path = (char *)malloc(len + 8);
    memcpy(path, appdata, len);
    memcpy(path + len, "/cubec", 7);
    path[len + 7] = '\0';
    return path;
  }
#else
  const char *xdg = getenv("XDG_CONFIG_HOME");
  if (xdg && xdg[0] != '\0') {
    size_t len = strlen(xdg);
    char *path = (char *)malloc(len + 8);
    memcpy(path, xdg, len);
    memcpy(path + len, "/cubec", 7);
    path[len + 7] = '\0';
    return path;
  }
  const char *home = getenv("HOME");
  if (home && home[0] != '\0') {
    size_t len = strlen(home);
    char *path = (char *)malloc(len + 16);
    memcpy(path, home, len);
    memcpy(path + len, "/.config/cubec", 15);
    path[len + 15] = '\0';
    return path;
  }
#endif
  return NULL;
}

int env_init(void) {
  g_env.cubec_home = discover_cubec_home();
  g_env.cc = discover_cc();
  g_env.ar = discover_ar();

  const char *cflags_env = getenv("CFLAGS");
  g_env.cflags = (cflags_env && cflags_env[0] != '\0') ? ENV_STRDUP(cflags_env) : ENV_STRDUP("");

  const char *ldflags_env = getenv("LDFLAGS");
  g_env.ldflags = (ldflags_env && ldflags_env[0] != '\0') ? ENV_STRDUP(ldflags_env) : ENV_STRDUP("");

  return 0;
}

const env_config_t *env_get(void) {
  return &g_env;
}

void env_dispose(void) {
  free((void *)g_env.cubec_home);
  free((void *)g_env.cc);
  free((void *)g_env.ar);
  free((void *)g_env.cflags);
  free((void *)g_env.ldflags);
  g_env.cubec_home = NULL;
  g_env.cc = NULL;
  g_env.ar = NULL;
  g_env.cflags = NULL;
  g_env.ldflags = NULL;
}
