cmake_minimum_required(VERSION 3.12)
project(cubec)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# ===========================================================================
# Third-party dependencies via git submodules
# ===========================================================================

# Google Test (v1.15.2) - force shared CRT to match ICU/cubec build
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/googletest EXCLUDE_FROM_ALL)

# ICU (release-74-2) - build from source with CMake
include(${PROJECT_SOURCE_DIR}/cmake/icu.cmake)

find_package(Threads REQUIRED)

# ===========================================================================
# Compile definitions & warning flags
# ===========================================================================
add_compile_definitions(_CRT_SECURE_NO_WARNINGS _GNU_SOURCE)

# Strict warning flags for C/C++
add_compile_options(
    -Wall -Wextra -Wpedantic
    -Werror=unused-function
    -Werror=unused-variable
    -Werror=unused-value
    -Werror=switch
    -Wno-nested-anon-types
    -Wno-c99-extensions
    -Wno-c23-extensions
    -Wno-zero-length-array
    -Wno-deprecated-declarations
    -Wno-missing-field-initializers
    -Wno-sign-compare
    -Wno-unused-parameter
    -Wno-unused-label
)
# CXX-specific: suppress unused-parameter and character-conversion in gtest headers
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-parameter>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-character-conversion>)
# Also suppress character-conversion in googletest (it triggers in gtest-printers.h)
target_compile_options(gtest PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-character-conversion>)
target_compile_options(gmock PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-character-conversion>)

# Optional: include-what-you-use for unused include detection
# Build with: cmake -DCMAKE_C_INCLUDE_WHAT_YOU_USE="include-what-you-use" ..

# ===========================================================================
# Build targets
# ===========================================================================
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/third_party/cJSON)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/cmd SRC_CMD)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/core SRC_CORE)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/engine SRC_ENGINE)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/cubec SRC_CUBEC)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/run SRC_RUN)
set(SRC_CJSON ${PROJECT_SOURCE_DIR}/third_party/cJSON/cJSON.c)
add_executable(cubec ${SRC_CMD} ${SRC_CORE} ${SRC_ENGINE} ${SRC_CUBEC} ${SRC_RUN} ${SRC_CJSON} ${PROJECT_SOURCE_DIR}/src/main.c ${PROJECT_SOURCE_DIR}/src/icu_data.c)
target_link_libraries(cubec PRIVATE
    ICU::i18n
    ICU::uc
    ICU::data
    ICU::io
    Threads::Threads
)
enable_testing()
file(GLOB_RECURSE TEST_SOURCE ${PROJECT_SOURCE_DIR}/test/*.cpp)
add_executable(cubec_test ${SRC_CORE} ${SRC_ENGINE} ${SRC_CUBEC} ${SRC_RUN} ${SRC_CJSON} ${TEST_SOURCE} ${PROJECT_SOURCE_DIR}/src/icu_data.c)
target_include_directories(cubec_test PUBLIC ${PROJECT_SOURCE_DIR}/test)
# Relax unused-variable/unused-function for test code (gtest patterns)
# Suppress character-conversion from googletest headers
target_compile_options(cubec_test PRIVATE -Wno-unused-variable -Wno-unused-function -Wno-character-conversion)
target_link_libraries(cubec_test PRIVATE
    ICU::i18n
    ICU::uc
    ICU::data
    ICU::io
    gtest
    gmock
    Threads::Threads
)
include(GoogleTest)
gtest_discover_tests(cubec_test)

# ===========================================================================
# Platform-specific: Windows Clang needs clang_rt.builtins
# ===========================================================================
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    message(STATUS "Target platform is Windows")

    if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" OR
        "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        message(STATUS "Compiler is Clang")
        target_link_libraries(cubec_test PRIVATE clang_rt.builtins-x86_64)
        target_link_libraries(cubec PRIVATE clang_rt.builtins-x86_64)
    endif()
endif()
