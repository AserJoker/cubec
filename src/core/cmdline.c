#include "core/cmdline.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cmd_parsed_t cmd_parse(const cmd_subcommand_t *cmds, int cmd_count,
                       int argc, char *argv[]) {
  cmd_parsed_t result = {NULL, NULL, 0, NULL, 0};

  if (argc < 2) return result;

  /* Find subcommand */
  const cmd_subcommand_t *cmd = NULL;
  for (int i = 0; i < cmd_count; i++) {
    if (strcmp(argv[1], cmds[i].name) == 0) {
      cmd = &cmds[i];
      break;
    }
  }
  if (!cmd) return result;

  /* Check for --help before full parsing */
  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      result.subcommand = cmd;
      result.help_requested = true;
      return result;
    }
  }

  /* Collect options and positional args */
  /* Max options = argc, allocate on stack */
  cmd_parsed_option_t opts[64];
  int opt_count = 0;
  const char *pos[64];
  int pos_count = 0;

  for (int i = 2; i < argc; i++) {
    if (argv[i][0] == '-') {
      /* Find matching option */
      const cmd_option_t *opt = NULL;
      for (int j = 0; j < cmd->option_count; j++) {
        if ((cmd->options[j].long_name &&
             strcmp(argv[i], cmd->options[j].long_name) == 0) ||
            (cmd->options[j].short_name &&
             strcmp(argv[i], cmd->options[j].short_name) == 0)) {
          opt = &cmd->options[j];
          break;
        }
      }
      if (!opt) {
        /* Unknown option — treat as positional if it's just "-" */
        if (argv[i][1] == '\0') {
          if (pos_count < 64) pos[pos_count++] = argv[i];
          continue;
        }
        fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
        result.subcommand = cmd; /* set so caller can generate usage */
        return result;
      }
      if (opt_count < 64) {
        opts[opt_count].name = opt->long_name ? opt->long_name : opt->short_name;
        opts[opt_count].value = NULL;
        if (opt->takes_arg) {
          if (i + 1 < argc) {
            opts[opt_count].value = argv[++i];
          } else {
            fprintf(stderr, "error: option '%s' requires an argument\n",
                    opts[opt_count].name);
            result.subcommand = cmd;
            return result;
          }
        }
        opt_count++;
      }
    } else {
      /* Positional argument */
      if (pos_count < 64) pos[pos_count++] = argv[i];
    }
  }

  /* Validate positional arg count */
  if (pos_count < cmd->min_positional) {
    fprintf(stderr, "error: '%s' requires at least %d positional argument(s), got %d\n",
            cmd->name, cmd->min_positional, pos_count);
    result.subcommand = cmd;
    return result;
  }
  if (cmd->max_positional >= 0 && pos_count > cmd->max_positional) {
    fprintf(stderr, "error: '%s' accepts at most %d positional argument(s), got %d\n",
            cmd->name, cmd->max_positional, pos_count);
    result.subcommand = cmd;
    return result;
  }

  /* Copy to heap for stable result */
  cmd_parsed_option_t *opts_heap = NULL;
  if (opt_count > 0) {
    opts_heap = (cmd_parsed_option_t *)malloc(sizeof(cmd_parsed_option_t) * opt_count);
    memcpy(opts_heap, opts, sizeof(cmd_parsed_option_t) * opt_count);
  }
  const char **pos_heap = NULL;
  if (pos_count > 0) {
    pos_heap = (const char **)malloc(sizeof(const char *) * pos_count);
    memcpy(pos_heap, pos, sizeof(const char *) * pos_count);
  }

  result.subcommand = cmd;
  result.options = opts_heap;
  result.option_count = opt_count;
  result.positional = pos_heap;
  result.positional_count = pos_count;
  return result;
}

const char *cmd_get_option(const cmd_parsed_t *parsed, const char *name) {
  if (!parsed || !parsed->options) return NULL;
  for (int i = 0; i < parsed->option_count; i++) {
    if (strcmp(parsed->options[i].name, name) == 0) {
      return parsed->options[i].value ? parsed->options[i].value : "";
    }
  }
  return NULL;
}

int cmd_usage(const cmd_subcommand_t *cmd, const char *prog_name,
              char *buf, int buf_size) {
  int written = 0;
  written += snprintf(buf + written, buf_size - written,
                      "usage: %s %s", prog_name, cmd->name);

  if (cmd->option_count > 0)
    written += snprintf(buf + written, buf_size - written, " [options]");

  if (cmd->max_positional == 1)
    written += snprintf(buf + written, buf_size - written, " <file>");
  else if (cmd->max_positional > 1)
    written += snprintf(buf + written, buf_size - written, " <args...>");
  else if (cmd->max_positional < 0)
    written += snprintf(buf + written, buf_size - written, " [args...]");

  written += snprintf(buf + written, buf_size - written, "\n\n  %s\n", cmd->help);

  if (cmd->option_count > 0) {
    written += snprintf(buf + written, buf_size - written, "\nOptions:\n");
    for (int i = 0; i < cmd->option_count; i++) {
      const cmd_option_t *opt = &cmd->options[i];
      if (opt->short_name)
        written += snprintf(buf + written, buf_size - written,
                            "  %s, %-12s %s\n",
                            opt->short_name, opt->long_name, opt->help);
      else
        written += snprintf(buf + written, buf_size - written,
                            "  %-16s %s\n", opt->long_name, opt->help);
    }
  }
  return written;
}

int cmd_dispatch(const cmd_subcommand_t *cmds, int cmd_count,
                 int argc, char *argv[]) {
  cmd_parsed_t parsed = cmd_parse(cmds, cmd_count, argc, argv);

  if (!parsed.subcommand) {
    fprintf(stderr, "usage: %s <command> [args]\n\nCommands:\n",
            argc > 0 ? argv[0] : "cubec");
    for (int i = 0; i < cmd_count; i++) {
      fprintf(stderr, "  %-12s %s\n", cmds[i].name, cmds[i].help);
    }
    return 1;
  }

  /* Check if --help was requested via --help/-h flag */
  if (parsed.help_requested) {
    char buf[512];
    cmd_usage(parsed.subcommand, argv[0], buf, sizeof(buf));
    fprintf(stderr, "%s", buf);
    return 0;
  }

  int ret = parsed.subcommand->run(&parsed);

  /* Free heap-allocated arrays */
  free((void *)parsed.options);
  free((void *)parsed.positional);

  return ret;
}
