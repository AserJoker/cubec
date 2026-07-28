#include "lsp/server.h"
#include "lsp/transport.h"
#include "lsp/handler.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

/* Exposed from handler.c */
extern bool lsp_handler_is_shutdown(void);

int lsp_server_run(void) {
  /* Set stdin/stdout to binary mode on Windows to avoid \r\n translation */
#ifdef _WIN32
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  /* Disable stdout buffering for immediate response delivery */
  setvbuf(stdout, NULL, _IONBF, 0);

  lsp_handler_init();

  int exit_code = 0;

  for (;;) {
    lsp_message_t *msg = lsp_transport_read();
    if (!msg) {
      /* EOF — client disconnected */
      break;
    }

    char *response = lsp_handle_request(msg->body);
    lsp_message_free(msg);

    if (response) {
      lsp_transport_write(response);
      free(response);
    }

    if (lsp_handler_is_shutdown()) {
      exit_code = 0;
      break;
    }
  }

  lsp_handler_dispose();
  return exit_code;
}
