#include "engine/flow_state.h"
#include <string.h>

flow_state_t flow_state_create(allocator_t allocator) {
  flow_state_t fs = (flow_state_t)allocator_alloc(allocator, sizeof(struct flow_state));
  if (!fs) return NULL;
  fs->term = FLOW_ALIVE;
  vec_init_t vi = {.auto_dispose = false};
  fs->tdz_set = (vec_t)allocator_create(allocator, &g_vec_type, &vi);
  return fs;
}

void flow_state_dispose(flow_state_t fs, allocator_t allocator) {
  if (!fs) return;
  /* Save tdz_set before freeing fs, because allocator_free unlinks from
     the chunk list and fs->last may point to the tdz_set chunk.
     Freeing fs first is safe: its chunk's last points to the still-valid
     tdz_set chunk. Then free tdz_set. */
  vec_t saved_tdz = fs->tdz_set;
  allocator_free(allocator, (void **)&fs);
  allocator_free(allocator, &saved_tdz);
}

flow_state_t flow_state_clone(allocator_t allocator, flow_state_t src) {
  if (!src) return flow_state_alive(allocator);
  flow_state_t fs = flow_state_create(allocator);
  if (!fs) return NULL;
  fs->term = src->term;
  size_t count = vec_get_size(src->tdz_set);
  for (size_t i = 0; i < count; i++) {
    const char *name = (const char *)vec_get(src->tdz_set, i);
    vec_push(fs->tdz_set, (void *)name);
  }
  return fs;
}

flow_state_t flow_state_alive(allocator_t allocator) {
  return flow_state_create(allocator);
}

void flow_state_mark_returned(flow_state_t fs) {
  if (fs) fs->term = FLOW_RETURNED;
}

void flow_state_mark_broke(flow_state_t fs) {
  if (fs) fs->term = FLOW_BROKE;
}

void flow_state_mark_continued(flow_state_t fs) {
  if (fs) fs->term = FLOW_CONTINUED;
}

void flow_state_add_tdz(flow_state_t fs, const char *name) {
  if (!fs || !name) return;
  /* Avoid duplicates (use strcmp since names come from different string_t objects) */
  if (flow_state_is_tdz(fs, name)) return;
  vec_push(fs->tdz_set, (void *)name);
}

void flow_state_remove_tdz(flow_state_t fs, const char *name) {
  if (!fs || !name) return;
  size_t count = vec_get_size(fs->tdz_set);
  for (size_t i = 0; i < count; i++) {
    const char *entry = (const char *)vec_get(fs->tdz_set, i);
    if (strcmp(entry, name) == 0) {
      vec_remove(fs->tdz_set, i);
      return;
    }
  }
}

bool flow_state_is_tdz(flow_state_t fs, const char *name) {
  if (!fs || !name) return false;
  size_t count = vec_get_size(fs->tdz_set);
  for (size_t i = 0; i < count; i++) {
    const char *entry = (const char *)vec_get(fs->tdz_set, i);
    if (strcmp(entry, name) == 0) return true;
  }
  return false;
}

flow_state_t flow_state_merge(allocator_t allocator,
                               flow_state_t a, flow_state_t b) {
  flow_state_t result = flow_state_create(allocator);
  if (!result) return NULL;

  /* Termination merge:
   * - Both ALIVE => ALIVE
   * - Both RETURNED => RETURNED
   * - One ALIVE + one RETURNED => ALIVE (some path doesn't return)
   * - BROKE/CONTINUED only relevant inside loops; merged away
   */
  if (a->term == FLOW_RETURNED && b->term == FLOW_RETURNED) {
    result->term = FLOW_RETURNED;
  } else {
    result->term = FLOW_ALIVE;
  }

  /* TDZ merge: union — variable is TDZ if it's TDZ in either branch */
  size_t a_count = vec_get_size(a->tdz_set);
  size_t b_count = vec_get_size(b->tdz_set);

  /* Add all from a */
  for (size_t i = 0; i < a_count; i++) {
    const char *name = (const char *)vec_get(a->tdz_set, i);
    vec_push(result->tdz_set, (void *)name);
  }
  /* Add from b only if not already present (from a) */
  for (size_t i = 0; i < b_count; i++) {
    const char *name = (const char *)vec_get(b->tdz_set, i);
    if (!flow_state_is_tdz(result, name)) {
      vec_push(result->tdz_set, (void *)name);
    }
  }

  return result;
}

bool flow_state_is_unreachable(flow_state_t fs) {
  return fs && (fs->term == FLOW_RETURNED || fs->term == FLOW_BROKE ||
                fs->term == FLOW_CONTINUED);
}

bool flow_state_is_all_returned(flow_state_t fs) {
  return fs && fs->term == FLOW_RETURNED;
}
