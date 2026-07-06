# =============================================================================
# ICU Build Module
# Builds ICU (release-74-2) from git submodule source via CMake.
# ICU common data (third_party/icudt74l.dat) is converted to a C byte array
# at build time. When upgrading ICU, replace this .dat file.
# =============================================================================

set(ICU_ROOT ${PROJECT_SOURCE_DIR}/third_party/icu/icu4c/source)

# ---------------------------------------------------------------------------
# Generate icu_data_gen.c from icudt74l.dat at build time
# ---------------------------------------------------------------------------
set(ICU_DATA_DAT   "${PROJECT_SOURCE_DIR}/third_party/icudt74l.dat")
set(ICU_DATA_GEN_C "${CMAKE_BINARY_DIR}/icu_data_gen.c")
set(BIN2C_SCRIPT   "${PROJECT_SOURCE_DIR}/cmake/bin_to_c.ps1")

if(WIN32)
    add_custom_command(
        OUTPUT "${ICU_DATA_GEN_C}"
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass
            -File "${BIN2C_SCRIPT}"
            -InputFile "${ICU_DATA_DAT}"
            -OutputFile "${ICU_DATA_GEN_C}"
            -VarName "icudt74l_dat"
            -Quiet
        DEPENDS "${ICU_DATA_DAT}" "${BIN2C_SCRIPT}"
        COMMENT "Converting icudt74l.dat to C byte array..."
        VERBATIM
    )
else()
    add_custom_command(
        OUTPUT "${ICU_DATA_GEN_C}"
        COMMAND sh -c "xxd -i -n icudt74l_dat '${ICU_DATA_DAT}' | sed 's/_len =/_size =/; s/_len\;/_size\;/g' > '${ICU_DATA_GEN_C}'"
        DEPENDS "${ICU_DATA_DAT}"
        COMMENT "Converting icudt74l.dat to C byte array..."
        VERBATIM
    )
endif()

# ---------------------------------------------------------------------------
# Collect source files
# ---------------------------------------------------------------------------
file(GLOB ICU_COMMON_SRC
    ${ICU_ROOT}/common/*.cpp
    ${ICU_ROOT}/common/*.c
)

file(GLOB ICU_I18N_SRC
    ${ICU_ROOT}/i18n/*.cpp
    ${ICU_ROOT}/i18n/*.c
)

file(GLOB ICU_IO_SRC
    ${ICU_ROOT}/io/*.cpp
    ${ICU_ROOT}/io/*.c
)

# ---------------------------------------------------------------------------
# PUBLIC defines - must be visible to both ICU and consumers
# ---------------------------------------------------------------------------
set(ICU_PUBLIC_DEFS
    U_STATIC_IMPLEMENTATION
    U_DISABLE_RENAMING=1
    U_DISABLE_VERSION_SUFFIX=1
    U_USING_ICU_NAMESPACE=0
)
if(WIN32)
    list(APPEND ICU_PUBLIC_DEFS
        U_PLATFORM_USES_ONLY_WIN32_API=1
        U_PLATFORM_HAS_WINUWP_API=0
    )
endif()

# ---------------------------------------------------------------------------
# PRIVATE defines
# ---------------------------------------------------------------------------
set(ICU_COMMON_DEF U_COMMON_IMPLEMENTATION)
set(ICU_I18N_DEF  U_I18N_IMPLEMENTATION)
set(ICU_IO_DEF    U_IO_IMPLEMENTATION)

# ---------------------------------------------------------------------------
# ICU Common Library (libicuuc)
# ---------------------------------------------------------------------------
add_library(icuuc STATIC ${ICU_COMMON_SRC})
target_include_directories(icuuc PUBLIC ${ICU_ROOT}/common)
target_compile_definitions(icuuc
    PUBLIC  ${ICU_PUBLIC_DEFS}
    PRIVATE ${ICU_COMMON_DEF}
)
if(MSVC)
    target_compile_definitions(icuuc PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ---------------------------------------------------------------------------
# ICU Data Library (libicudata)
#   - icu_data_gen.c: icudt74l.dat embedded as C byte array
#   - stubdata.cpp: minimal stub (provides entry points)
#   - icudataver.cpp: data version info
# ---------------------------------------------------------------------------
add_library(icudata STATIC
    "${ICU_DATA_GEN_C}"
    ${ICU_ROOT}/stubdata/stubdata.cpp
    ${ICU_ROOT}/common/icudataver.cpp
)
target_include_directories(icudata PUBLIC ${ICU_ROOT}/common)
target_compile_definitions(icudata
    PUBLIC  ${ICU_PUBLIC_DEFS}
)
if(MSVC)
    target_compile_definitions(icudata PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ---------------------------------------------------------------------------
# ICU I18N Library (libicui18n)
# ---------------------------------------------------------------------------
add_library(icui18n STATIC ${ICU_I18N_SRC})
target_include_directories(icui18n PUBLIC ${ICU_ROOT}/i18n)
target_link_libraries(icui18n PUBLIC icuuc icudata)
target_compile_definitions(icui18n
    PUBLIC  ${ICU_PUBLIC_DEFS}
    PRIVATE ${ICU_I18N_DEF}
)
if(MSVC)
    target_compile_definitions(icui18n PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ---------------------------------------------------------------------------
# ICU IO Library (libicuio)
# ---------------------------------------------------------------------------
add_library(icuio STATIC ${ICU_IO_SRC})
target_include_directories(icuio PUBLIC ${ICU_ROOT}/io)
target_link_libraries(icuio PUBLIC icuuc icui18n)
target_compile_definitions(icuio
    PUBLIC  ${ICU_PUBLIC_DEFS}
    PRIVATE ${ICU_IO_DEF}
)
if(MSVC)
    target_compile_definitions(icuio PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ---------------------------------------------------------------------------
# Alias targets matching vcpkg convention
# ---------------------------------------------------------------------------
add_library(ICU::uc   ALIAS icuuc)
add_library(ICU::i18n ALIAS icui18n)
add_library(ICU::data ALIAS icudata)
add_library(ICU::io   ALIAS icuio)

# ---------------------------------------------------------------------------
# Suppress ICU warnings
# ---------------------------------------------------------------------------
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    foreach(_tgt icuuc icudata icui18n icuio)
        target_compile_options(${_tgt} PRIVATE -w)
    endforeach()
elseif(MSVC)
    foreach(_tgt icuuc icudata icui18n icuio)
        target_compile_options(${_tgt} PRIVATE /W0)
    endforeach()
endif()

message(STATUS "ICU build targets configured (embedded data mode)")
