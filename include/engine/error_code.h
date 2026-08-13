#ifndef _H_CUBEC_ENGINE_ERROR_CODE_
#define _H_CUBEC_ENGINE_ERROR_CODE_
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Built-in error codes for the user-facing error struct.
 *
 * Stored in error.error_code (u64). User-defined error codes should
 * start from ERROR_CODE_USER_BEGIN to avoid collisions with built-ins.
 */

/* ---- No error ---- */
#define ERROR_CODE_NONE          ((uint64_t)0)

/* ---- Union errors ---- */
#define ERROR_CODE_UNION_INACTIVE_FIELD  ((uint64_t)1)  /* access inactive variant */
#define ERROR_CODE_UNION_ADDR_INACTIVE   ((uint64_t)2)  /* take address of inactive variant */

/* ---- User-defined error codes start here ---- */
#define ERROR_CODE_USER_BEGIN    ((uint64_t)1000)

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_ERROR_CODE_ */
