#include "core/cmdline.h"
#include "core/env.h"
#include "core/icu_data.h"
#include <stddef.h>

int _main(int argc, char *argv[]) {
  icu_data_init();
  env_init();
  int ret = cmd_dispatch(NULL, 0, argc, argv);
  env_dispose();
  return ret;
}

int main(int argc, char *argv[]) {
  return _main(argc, argv);
}
