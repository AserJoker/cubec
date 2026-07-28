#include "builder/compile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define ENV_STRDUP _strdup
#else
#include <sys/stat.h>
#include <unistd.h>
#define ENV_STRDUP strdup
#endif

/* Execute a command, returns exit code. Uses CreateProcess on Windows
   to avoid shell quoting issues, system() on POSIX. */
static int run_command(const char *cmd) {
#ifdef _WIN32
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  memset(&pi, 0, sizeof(pi));

  /* CreateProcess needs a mutable command line */
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

bool compile_is_up_to_date(const char *obj_path, const char *src_path) {
#ifdef _WIN32
  WIN32_FILE_ATTRIBUTE_DATA src_info, obj_info;
  if (!GetFileAttributesExA(src_path, GetFileExInfoStandard, &src_info))
    return false;
  if (!GetFileAttributesExA(obj_path, GetFileExInfoStandard, &obj_info))
    return false;
  ULARGE_INTEGER src_time, obj_time;
  src_time.LowPart = src_info.ftLastWriteTime.dwLowDateTime;
  src_time.HighPart = src_info.ftLastWriteTime.dwHighDateTime;
  obj_time.LowPart = obj_info.ftLastWriteTime.dwLowDateTime;
  obj_time.HighPart = obj_info.ftLastWriteTime.dwHighDateTime;
  return obj_time.QuadPart >= src_time.QuadPart;
#else
  struct stat src_st, obj_st;
  if (stat(src_path, &src_st) != 0) return false;
  if (stat(obj_path, &obj_st) != 0) return false;
  return obj_st.st_mtime >= src_st.st_mtime;
#endif
}

int compile_source(const char *src_path, const char *build_dir,
                   const char *cc, const char *cflags,
                   char **out_obj_path) {
  /* Derive object file path: build_dir/<basename>.o */
  const char *slash = strrchr(src_path, '/');
  const char *bslash = strrchr(src_path, '\\');
  const char *base = src_path;
  if (slash && slash + 1 > base) base = slash + 1;
  if (bslash && bslash + 1 > base) base = bslash + 1;

  const char *dot = strrchr(base, '.');
  size_t base_len = dot ? (size_t)(dot - base) : strlen(base);

  size_t dir_len = strlen(build_dir);
  char *obj_path = (char *)malloc(dir_len + 1 + base_len + 3);
  memcpy(obj_path, build_dir, dir_len);
  obj_path[dir_len] = '/';
  memcpy(obj_path + dir_len + 1, base, base_len);
  memcpy(obj_path + dir_len + 1 + base_len, ".o", 3);

  /* Incremental: skip if .o is up to date */
  if (compile_is_up_to_date(obj_path, src_path)) {
    *out_obj_path = obj_path;
    return 0;
  }

  /* Build command: "cc" -c [cflags] -o "obj_path" "src_path" */
  size_t cmd_len = strlen(cc) + strlen(cflags) + strlen(obj_path) + strlen(src_path) + 20;
  char *cmd = (char *)malloc(cmd_len);
  if (!cmd) { free(obj_path); return 1; }
  snprintf(cmd, cmd_len, "\"%s\" -c %s -o \"%s\" \"%s\"", cc, cflags, obj_path, src_path);

  int ret = run_command(cmd);
  free(cmd);

  if (ret != 0) {
    fprintf(stderr, "error: compilation failed for '%s'\n", src_path);
    free(obj_path);
    return 1;
  }

  *out_obj_path = obj_path;
  return 0;
}
