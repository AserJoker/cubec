#include "lsp/diagnostics.h"

#include "core/allocator.h"
#include "core/strmap.h"
#include "engine/context.h"
#include "engine/diagnostic.h"
#include "engine/source.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include <cJSON.h>
#include <stdlib.h>
#include <string.h>

/* Convert a cubec position (0-based) to LSP Position (0-based) — identity */
static cJSON *make_position(size_t line, size_t column) {
  cJSON *pos = cJSON_CreateObject();
  cJSON_AddNumberToObject(pos, "line", line);
  cJSON_AddNumberToObject(pos, "character", column);
  return pos;
}

/* Convert a cubec location to LSP Range */
static cJSON *make_range(location_t loc) {
  cJSON *range = cJSON_CreateObject();
  cJSON_AddItemToObject(range, "start", make_position(loc.begin.line, loc.begin.column));
  cJSON_AddItemToObject(range, "end", make_position(loc.end.line, loc.end.column));
  return range;
}

/* Map cubec severity to LSP DiagnosticSeverity */
static int severity_to_lsp(enum diagnostic_severity sev) {
  switch (sev) {
    case DIAGNOSTIC_ERROR:   return 1; /* Error */
    case DIAGNOSTIC_WARNING: return 2; /* Warning */
    case DIAGNOSTIC_NOTE:    return 3; /* Information */
    default:                 return 3;
  }
}

cJSON *lsp_diagnostics_for_file(const char *uri, const char *source) {
  allocator_t allocator = create_allocator(NULL, NULL);
  context_t ctx = context_create(allocator);

  /* Use uri as filename for the context */
  ctx->current_file = uri;
  source_cache_load(ctx->sources, uri, source, false);

  /* Lex */
  vec_t tokens = resolve_token_list(ctx, uri, source);

  /* Parse */
  node_t program = NULL;
  if (tokens) {
    size_t position = 0;
    program = read_program_node(ctx, tokens, &position, uri);
  }

  /* Check */
  if (program) {
    context_check_program(ctx, program);
  }

  /* Collect diagnostics */
  cJSON *diags = cJSON_CreateArray();
  size_t count = diagnostic_list_get_size(ctx->diagnostics);
  for (size_t i = 0; i < count; i++) {
    struct diagnostic *d = diagnostic_list_get(ctx->diagnostics, i);
    cJSON *diag = cJSON_CreateObject();
    cJSON_AddNumberToObject(diag, "severity", severity_to_lsp(d->severity));
    cJSON_AddStringToObject(diag, "message", d->message);
    cJSON_AddItemToObject(diag, "range", make_range(d->primary));
    cJSON_AddStringToObject(diag, "source", "cubec");

    /* Add related information from notes */
    if (d->notes && vec_get_size(d->notes) > 0) {
      cJSON *related = cJSON_CreateArray();
      size_t note_count = vec_get_size(d->notes);
      for (size_t j = 0; j < note_count; j++) {
        struct diagnostic_note *note = vec_get(d->notes, j);
        cJSON *rel = cJSON_CreateObject();
        cJSON_AddStringToObject(rel, "message", note->message);
        cJSON *loc = cJSON_CreateObject();
        cJSON_AddStringToObject(loc, "uri", note->location.filename ? note->location.filename : uri);
        cJSON_AddItemToObject(loc, "range", make_range(note->location));
        cJSON_AddItemToObject(rel, "location", loc);
        cJSON_AddItemToArray(related, rel);
      }
      cJSON_AddItemToObject(diag, "relatedInformation", related);
    }

    cJSON_AddItemToArray(diags, diag);
  }

  /* Cleanup */
  if (program) allocator_free(allocator, &program);
  if (tokens) allocator_free(allocator, &tokens);
  context_dispose(ctx);
  delete_allocator(allocator);

  return diags;
}
