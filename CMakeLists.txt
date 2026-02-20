cmake_minimum_required(VERSION 3.12)
set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)
project(cubec)
add_compile_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_compile_options(-Wno-trigraphs)
find_package(ICU REQUIRED COMPONENTS i18n uc data io)
find_package(GTest CONFIG REQUIRED)
include_directories(${PROJECT_SOURCE_DIR}/include)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/core SRC_CORE)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/ast SRC_AST)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/astwriter SRC_ASTWRITER)
add_executable(cubec ${SRC_CORE} ${SRC_AST} ${SRC_ASTWRITER} ${PROJECT_SOURCE_DIR}/src/cmd/cubec.c)
target_link_libraries(cubec PRIVATE
    ICU::i18n
    ICU::uc
    ICU::data
    ICU::io
)
enable_testing()
file(GLOB_RECURSE TEST_SOURCE ${PROJECT_SOURCE_DIR}/test/*.cpp)
add_executable(cubec_test ${SRC_CORE} ${SRC_AST} ${SRC_ASTWRITER} ${TEST_SOURCE})
target_link_libraries(cubec_test PRIVATE
    ICU::i18n
    ICU::uc
    ICU::data
    ICU::io
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main)
include(GoogleTest)
gtest_discover_tests(cubec_test)