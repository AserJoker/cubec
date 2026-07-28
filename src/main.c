#include "core/cmdline.h"
#include "core/env.h"
#include "core/icu_data.h"
#include "cmd/cmd_build.h"
#include "cmd/cmd_test.h"

static cmd_subcommand_t commands[2];

int _main(int argc, char *argv[]) {
  icu_data_init();
  env_init();
  commands[0] = cmd_build_def;
  commands[1] = cmd_test_def;
  int ret = cmd_dispatch(commands, 2, argc, argv);
  env_dispose();
  return ret;
}

int main(int argc, char *argv[]) {
  return _main(argc, argv);
}
