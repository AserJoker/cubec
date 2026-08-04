#include "cmd/format.h"
#include "core/cmdline.h"
#include "core/env.h"
#include "core/icu_data.h"
#include <stddef.h>

int _main(int argc, char *argv[]) {
  icu_data_init();
  env_init();
  const cmd_subcommand_t cmds[] = {cmd_format};
  int ret = cmd_dispatch(cmds, 1, argc, argv);
  env_dispose();
  return ret;
}

int main(int argc, char *argv[]) {
  return _main(argc, argv);
}
