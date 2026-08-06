#include "core/strmap.h"
#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/node.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include "engine/def.h"
#include "engine/def_collector.h"
#include "engine/name.h"
#include "engine/name_collector.h"
#include "engine/module.h"
#include "engine/scope.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

class it_def_collector : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;

  /** Parse source, run name + def collection, returning the module. */
  module_t parse_and_collect(const char *source, const char *filename) {
    char *owned_source = strdup(source);
    vec_t tokens = resolve_token_list(ctx, filename, owned_source);
    EXPECT_NE(tokens, nullptr);
    size_t pos = 0;
    node_t program = read_program_node(ctx, tokens, &pos, filename);
    EXPECT_NE(program, nullptr);
    module_t mod =
        module_create(allocator, ctx->global_scope, filename, owned_source, tokens, program);
    name_collector_run(ctx, mod);
    def_collector_run(ctx, mod);
    return mod;
  }
};

/* ---- module state after def collection ---- */

TEST_F(it_def_collector, module_state_resolved) {
  const char *source = "func foo(): void {}";
  module_t mod = parse_and_collect(source, "test.cubec");

  EXPECT_EQ(mod->state, MODULE_RESOLVED);

  module_dispose(mod);
}

/* ---- idempotent: running again is a no-op ---- */

TEST_F(it_def_collector, idempotent) {
  const char *source = "func foo(): void {}";
  module_t mod = parse_and_collect(source, "test.cubec");

  def_collector_run(ctx, mod); /* should be a no-op */
  EXPECT_EQ(mod->state, MODULE_RESOLVED);

  module_dispose(mod);
}
