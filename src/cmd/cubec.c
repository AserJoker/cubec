
#include "ast/node.h"
#include "core/allocator.h"
#include "core/path.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/module.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

char *absolute(allocator_t allocator, const char *name) {
  path_t path = create_path(allocator, name);
  path_t fullpath = path_absolute(path, allocator);
  char *result = path_to_string(fullpath, allocator);
  allocator_free(allocator, path);
  allocator_free(allocator, fullpath);
  return result;
}

static ast_node_t builtin_import(context_t ctx, ast_node_t node) {
  ast_node_t arguments = ast_get_child(node, "arguments");
  allocator_t allocator = context_get_allocator(ctx);
  if (ast_get_length(arguments) != 1) {
    value_t err = create_comptime_error(
        ctx, node, "import require 1 arguments,but received %" PRIuPTR,
        ast_get_length(arguments));
    return create_ast_value_node(allocator, err);
  }
  value_t src = resolve_expression(ctx, ast_get_item(arguments, 0));
  if (value_is_error(src) || value_is_interrupt(src)) {
    return create_ast_value_node(allocator, src);
  }
  if (type_get_kind(value_get_type(src)) != TYPE_KIND_STR) {
    value_t err = create_comptime_error(ctx, ast_get_item(arguments, 0),
                                        "import source is not a string");
    return create_ast_value_node(allocator, err);
  }
  const char *source = *(const char **)value_get_data(src);
  module_t mod = context_get_module(ctx);
  const char *dirname = module_get_dirname(mod);
  path_t pat = create_path(allocator, dirname);
  path_t cur = create_path(allocator, source);
  path_t full = path_concat(pat, allocator, cur);
  char *fullpath = path_to_string(full, allocator);
  allocator_free(allocator, full);
  allocator_free(allocator, cur);
  allocator_free(allocator, pat);
  value_t global = context_load_module(ctx, fullpath);
  allocator_free(allocator, fullpath);
  if (value_is_error(global)) {
    global = convert_comptime_error(ctx, node, global);
    return create_ast_value_node(allocator, global);
  }
  return create_ast_value_node(allocator, global);
}

int main(int argc, char *argv[]) {
  allocator_t allocator = create_allocator(NULL);
  context_t ctx = create_context(allocator);
  context_set_builtin(ctx, "import", builtin_import);
  char *filename = absolute(allocator, "./main.cubec");
  value_t err = context_load_module(ctx, filename);
  if (type_get_kind(value_get_type(err)) == TYPE_KIND_ERROR) {
    fprintf(stderr, "%s\n", error_get_message(err));
  } else {
    string_t out = context_write_module(ctx, filename);
    char *dst_filename = absolute(allocator, "./main.resolved.cubec");
    const char *str = string_get(out);
    FILE *fp = fopen(dst_filename, "w");
    fwrite(str, string_len(out), 1, fp);
    fclose(fp);
    allocator_free(allocator, dst_filename);
    allocator_free(allocator, out);
  }
  allocator_free(allocator, filename);
  allocator_free(allocator, ctx);
  delete_allocator(allocator);
  return 0;
}