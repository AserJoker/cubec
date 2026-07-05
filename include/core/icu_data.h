#ifndef ICU_DATA_H
#define ICU_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ICU common data from embedded binary.
 * Must be called once at startup before any ICU operations.
 * Returns 0 on success, -1 on failure.
 */
int icu_data_init(void);

#ifdef __cplusplus
}
#endif

#endif // ICU_DATA_H
