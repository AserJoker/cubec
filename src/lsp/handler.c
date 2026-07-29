#include "lsp/handler.h"
#include "lsp/transport.h"

#include "core/allocator.h"
#include "core/strmap.h"
#include <cJSON.h>
#include <string.h>

#ifdef _WIN32
#define ENV_STRDUP _strdup
#else
#define ENV_STRDUP strdup
#endif

/* ===== Document store ===== */

static allocator_t g_doc_allocator;
static strmap_t g_documents;
static bool g_initialized = false;
static bool g_shutdown = false;

void lsp_handler_init(void) {
  g_doc_allocator = create_allocator(NULL, NULL);
  g_documents = allocator_create(g_doc_allocator, &g_strmap_type,
                                  &(strmap_init_t){.value_auto_dispose = true});
  g_initialized = false;
  g_shutdown = false;
}

void lsp_handler_dispose(void) {
  allocator_free(g_doc_allocator, &g_documents);
  delete_allocator(g_doc_allocator);
}

/* ===== Document operations ===== */

static void doc_open(const char *uri, const char *text) {
  strmap_insert(g_documents, uri, ENV_STRDUP(text));
}

static void doc_change(const char *uri, const char *text) {
  strmap_insert(g_documents, uri, ENV_STRDUP(text));
}

static const char *doc_get(const char *uri) {
  return (const char *)strmap_find(g_documents, uri);
}

static void doc_close(const char *uri) {
  strmap_remove(g_documents, uri);
}

/* ===== LSP method handlers ===== */

static char *handle_initialize(cJSON *id) {
  cJSON *result = cJSON_CreateObject();
  cJSON *caps = cJSON_CreateObject();

  cJSON *sync = cJSON_CreateObject();
  cJSON_AddNumberToObject(sync, "openClose", 1);
  cJSON_AddNumberToObject(sync, "change", 1);
  cJSON_AddItemToObject(caps, "textDocumentSync", sync);

  cJSON_AddBoolToObject(caps, "definitionProvider", true);
  cJSON_AddBoolToObject(caps, "hoverProvider", true);

  cJSON_AddItemToObject(result, "capabilities", caps);

  cJSON *response = cJSON_CreateObject();
  cJSON_AddStringToObject(response, "jsonrpc", "2.0");
  cJSON_AddItemToObject(response, "id", id ? cJSON_Duplicate(id, true) : cJSON_CreateNull());
  cJSON_AddItemToObject(response, "result", result);

  char *out = cJSON_PrintUnformatted(response);
  cJSON_Delete(response);
  return out;
}

static char *handle_shutdown(cJSON *id) {
  g_shutdown = true;
  cJSON *response = cJSON_CreateObject();
  cJSON_AddStringToObject(response, "jsonrpc", "2.0");
  cJSON_AddItemToObject(response, "id", id ? cJSON_Duplicate(id, true) : cJSON_CreateNull());
  cJSON_AddNullToObject(response, "result");
  char *out = cJSON_PrintUnformatted(response);
  cJSON_Delete(response);
  return out;
}

/* Forward declarations */
extern cJSON *lsp_diagnostics_for_file(const char *uri, const char *source);
#include "lsp/definition.h"
#include "lsp/hover.h"

/* Expose doc_get for definition.c / hover.c */
const char *lsp_handler_doc_get(const char *uri) {
  return doc_get(uri);
}

static char *build_diagnostics_notification(const char *uri) {
  const char *source = doc_get(uri);
  if (!source) return NULL;

  cJSON *params = cJSON_CreateObject();
  cJSON_AddStringToObject(params, "uri", uri);
  cJSON_AddItemToObject(params, "diagnostics", lsp_diagnostics_for_file(uri, source));

  cJSON *notification = cJSON_CreateObject();
  cJSON_AddStringToObject(notification, "jsonrpc", "2.0");
  cJSON_AddStringToObject(notification, "method", "textDocument/publishDiagnostics");
  cJSON_AddItemToObject(notification, "params", params);

  char *out = cJSON_PrintUnformatted(notification);
  cJSON_Delete(notification);
  return out;
}

static char *handle_definition(cJSON *id, cJSON *params) {
  cJSON *td = cJSON_GetObjectItem(params, "textDocument");
  cJSON *pos = cJSON_GetObjectItem(params, "position");
  if (!td || !pos) return NULL;

  const char *uri = cJSON_GetObjectItem(td, "uri")->valuestring;
  int line = cJSON_GetObjectItem(pos, "line")->valueint;
  int character = cJSON_GetObjectItem(pos, "character")->valueint;

  cJSON *result = lsp_definition_for_position(uri, line, character);

  cJSON *response = cJSON_CreateObject();
  cJSON_AddStringToObject(response, "jsonrpc", "2.0");
  cJSON_AddItemToObject(response, "id", id ? cJSON_Duplicate(id, true) : cJSON_CreateNull());
  cJSON_AddItemToObject(response, "result", result ? result : cJSON_CreateNull());

  char *out = cJSON_PrintUnformatted(response);
  cJSON_Delete(response);
  return out;
}

static char *handle_hover(cJSON *id, cJSON *params) {
  cJSON *td = cJSON_GetObjectItem(params, "textDocument");
  cJSON *pos = cJSON_GetObjectItem(params, "position");
  if (!td || !pos) return NULL;

  const char *uri = cJSON_GetObjectItem(td, "uri")->valuestring;
  int line = cJSON_GetObjectItem(pos, "line")->valueint;
  int character = cJSON_GetObjectItem(pos, "character")->valueint;

  cJSON *result = lsp_hover_for_position(uri, line, character);

  cJSON *response = cJSON_CreateObject();
  cJSON_AddStringToObject(response, "jsonrpc", "2.0");
  cJSON_AddItemToObject(response, "id", id ? cJSON_Duplicate(id, true) : cJSON_CreateNull());
  cJSON_AddItemToObject(response, "result", result ? result : cJSON_CreateNull());

  char *out = cJSON_PrintUnformatted(response);
  cJSON_Delete(response);
  return out;
}

/* ===== Main handler dispatch ===== */

char *lsp_handle_request(const char *json_body) {
  cJSON *msg = cJSON_Parse(json_body);
  if (!msg) return NULL;

  cJSON *id = cJSON_GetObjectItem(msg, "id");
  cJSON *method = cJSON_GetObjectItem(msg, "method");
  cJSON *params = cJSON_GetObjectItem(msg, "params");

  char *response = NULL;

  if (method && method->valuestring) {
    const char *m = method->valuestring;

    if (strcmp(m, "initialize") == 0) {
      g_initialized = true;
      response = handle_initialize(id);
    } else if (strcmp(m, "initialized") == 0) {
      /* Notification — no response */
    } else if (strcmp(m, "textDocument/didOpen") == 0) {
      if (params) {
        cJSON *td = cJSON_GetObjectItem(params, "textDocument");
        if (td) {
          const char *uri = cJSON_GetObjectItem(td, "uri")->valuestring;
          const char *text = cJSON_GetObjectItem(td, "text")->valuestring;
          doc_open(uri, text);
          response = build_diagnostics_notification(uri);
        }
      }
    } else if (strcmp(m, "textDocument/didChange") == 0) {
      if (params) {
        cJSON *td = cJSON_GetObjectItem(params, "textDocument");
        if (td) {
          const char *uri = cJSON_GetObjectItem(td, "uri")->valuestring;
          cJSON *changes = cJSON_GetObjectItem(params, "contentChanges");
          if (changes && cJSON_GetArraySize(changes) > 0) {
            cJSON *last = cJSON_GetArrayItem(changes, cJSON_GetArraySize(changes) - 1);
            const char *text = cJSON_GetObjectItem(last, "text")->valuestring;
            doc_change(uri, text);
          }
          response = build_diagnostics_notification(uri);
        }
      }
    } else if (strcmp(m, "textDocument/didClose") == 0) {
      if (params) {
        cJSON *td = cJSON_GetObjectItem(params, "textDocument");
        if (td) {
          doc_close(cJSON_GetObjectItem(td, "uri")->valuestring);
        }
      }
    } else if (strcmp(m, "textDocument/definition") == 0) {
      response = handle_definition(id, params);
    } else if (strcmp(m, "textDocument/hover") == 0) {
      response = handle_hover(id, params);
    } else if (strcmp(m, "shutdown") == 0) {
      response = handle_shutdown(id);
    } else if (strcmp(m, "exit") == 0) {
      /* Exit handled by server loop */
    }
  }

  cJSON_Delete(msg);
  return response;
}

bool lsp_handler_is_shutdown(void) {
  return g_shutdown;
}
