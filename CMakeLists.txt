cmake_minimum_required(VERSION 3.12)
set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
project(cubec)
add_compile_options(-rdynamic -O0 -g)
add_compile_definitions(-D_CRT_SECURE_NO_WARNINGS -D_GNU_SOURCE)
find_package(ICU REQUIRED COMPONENTS i18n uc data io)
find_package(GTest CONFIG REQUIRED)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/core SRC_CORE)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/engine SRC_ENGINE)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/writer SRC_WRITE)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/reader SRC_READER)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/cubec SRC_CUBEC)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/c SRC_C)
add_executable(cubecc ${SRC_CORE} ${SRC_ENGINE} ${SRC_CUBEC} ${SRC_READER} ${SRC_WRITE} ${SRC_C} ${PROJECT_SOURCE_DIR}/src/main.c)
target_link_libraries(cubecc PRIVATE
    ICU::i18n
    ICU::uc
    ICU::data
    ICU::io
    dl
)
enable_testing()
file(GLOB_RECURSE TEST_SOURCE ${PROJECT_SOURCE_DIR}/test/*.cpp)
add_executable(cubec_test ${SRC_CORE} ${SRC_ENGINE} ${SRC_CUBEC} ${SRC_READER} ${SRC_WRITE} ${SRC_C} ${TEST_SOURCE})
target_include_directories(cubec_test PUBLIC ${PROJECT_SOURCE_DIR}/test)
target_link_libraries(cubec_test PRIVATE
    ICU::i18n
    ICU::uc
    ICU::data
    ICU::io
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    dl
)
include(GoogleTest)
gtest_discover_tests(cubec_test)

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    message(STATUS "Target platform is Windows")

    if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" OR
        "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        message(STATUS "Compiler is Clang")
        target_link_libraries(cubec_test PRIVATE clang_rt.builtins-x86_64)
        target_link_libraries(cubec PRIVATE clang_rt.builtins-x86_64)
    endif()
endif()