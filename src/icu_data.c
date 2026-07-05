#include "core/icu_data.h"
#include "unicode/udata.h"
#include "unicode/utypes.h"
#include <stdlib.h>

/* Generated C byte array from icudt74l.dat (see cmake/icu.cmake) */
extern const unsigned char icudt74l_dat[];
extern const size_t icudt74l_dat_size;

int icu_data_init(void) {
    UErrorCode status = U_ZERO_ERROR;
    udata_setCommonData(icudt74l_dat, &status);
    return U_FAILURE(status) ? -1 : 0;
}
