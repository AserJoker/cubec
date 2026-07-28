#include "builder/link.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define ENV_STRDUP _strdup
#else
#define ENV_STRDUP strdup
#endif

/* Execute a command via CreateProcess (Windows) or system() (POSIX) */
static int run_command(const char *cmd) {
#ifdef _WIN32
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  memset(&pi, 0, sizeof(pi));

  char *cmdline = ENV_STRDUP(cmd);
  if (!cmdline) return 1;

  if (!CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0,
                       NULL, NULL, &si, &pi)) {
    free(cmdline);
    return 1;
  }
  free(cmdline);

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exit_code = 1;
  GetExitCodeProcess(pi.hProcess, &exit_code);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return (int)exit_code;
#else
  return system(cmd);
#endif
}

int link_executable(const char **obj_paths, int obj_count,
                    const char *output_path,
                    const char *cc, const char *ldflags) {
  size_t cmd_len = strlen(cc) + strlen(output_path) + strlen(ldflags) + 20;
  for (int i = 0; i < obj_count; i++) {
    cmd_len += strlen(obj_paths[i]) + 4;
  }

  char *cmd = (char *)malloc(cmd_len);
  if (!cmd) return 1;

  int pos = snprintf(cmd, cmd_len, "\"%s\" -o \"%s\"", cc, output_path);
  for (int i = 0; i < obj_count; i++) {
    pos += snprintf(cmd + pos, cmd_len - pos, " \"%s\"", obj_paths[i]);
  }
  if (ldflags[0] != '\0') {
    pos += snprintf(cmd + pos, cmd_len - pos, " %s", ldflags);
  }

  int ret = run_command(cmd);
  free(cmd);

  if (ret != 0) {
    fprintf(stderr, "error: linking failed for '%s'\n", output_path);
    return 1;
  }
  return 0;
}

int link_static_lib(const char **obj_paths, int obj_count,
                    const char *output_path, const char *ar_path) {
  const char *ar = ar_path ? ar_path : "ar";

  size_t cmd_len = strlen(ar) + strlen(output_path) + 10;
  for (int i = 0; i < obj_count; i++) {
    cmd_len += strlen(obj_paths[i]) + 4;
  }

  char *cmd = (char *)malloc(cmd_len);
  if (!cmd) return 1;

  int pos = snprintf(cmd, cmd_len, "\"%s\" rcs \"%s\"", ar, output_path);
  for (int i = 0; i < obj_count; i++) {
    pos += snprintf(cmd + pos, cmd_len - pos, " \"%s\"", obj_paths[i]);
  }

  int ret = run_command(cmd);
  free(cmd);

  if (ret != 0) {
    fprintf(stderr, "error: archiving failed for '%s'\n", output_path);
    return 1;
  }
  return 0;
}
