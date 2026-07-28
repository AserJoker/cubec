#ifndef _H_CUBEC_CORE_CMDLINE_
#define _H_CUBEC_CORE_CMDLINE_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Option definition */
typedef struct {
  const char *long_name;  /* "--library" */
  const char *short_name; /* "-l" or NULL */
  const char *help;       /* help text */
  bool takes_arg;         /* whether option takes a value */
} cmd_option_t;

/* Forward declaration for subcommand (needs cmd_parsed_t) */
typedef struct cmd_parsed_t cmd_parsed_t;

/* Subcommand definition */
typedef struct {
  const char *name;             /* "build", "test" */
  const char *help;             /* help text */
  const cmd_option_t *options;  /* option array, terminated by {NULL} entry */
  int option_count;
  int min_positional;           /* minimum positional args required */
  int max_positional;           /* maximum positional args, -1 for unlimited */
  int (*run)(const cmd_parsed_t *parsed); /* command handler */
} cmd_subcommand_t;

/* Parsed option */
typedef struct {
  const char *name;  /* matched long_name (or short_name if no long) */
  const char *value; /* option value for takes_arg, NULL for flags */
} cmd_parsed_option_t;

/* Full parse result */
struct cmd_parsed_t {
  const cmd_subcommand_t *subcommand;
  const cmd_parsed_option_t *options;
  int option_count;
  const char *const *positional;
  int positional_count;
  bool help_requested;
};

/* Parse command line. Returns result; on failure subcommand == NULL */
cmd_parsed_t cmd_parse(const cmd_subcommand_t *cmds, int cmd_count,
                       int argc, char *argv[]);

/* Look up option in parse result. Returns value ("" for flag), or NULL */
const char *cmd_get_option(const cmd_parsed_t *parsed, const char *name);

/* Generate usage text into buf. Returns chars written */
int cmd_usage(const cmd_subcommand_t *cmd, const char *prog_name,
              char *buf, int buf_size);

/* Convenience: parse + dispatch, returns command exit code */
int cmd_dispatch(const cmd_subcommand_t *cmds, int cmd_count,
                 int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_CORE_CMDLINE_ */
