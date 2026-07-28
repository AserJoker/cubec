#include "cmd/cmd_lsp.h"
#include "lsp/server.h"
#include <stddef.h>

static int cmd_lsp_run(const cmd_parsed_t *parsed) {
  (void)parsed;
  return lsp_server_run();
}

static const cmd_option_t lsp_options[] = {
  {NULL, NULL, NULL, false},
};

const cmd_subcommand_t cmd_lsp_def = {
  .name = "lsp",
  .help = "Start LSP language server",
  .options = lsp_options,
  .option_count = 0,
  .min_positional = 0,
  .max_positional = 0,
  .run = cmd_lsp_run,
};
