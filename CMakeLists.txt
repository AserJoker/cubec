cmake_minimum_required(VERSION 3.12)
set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
set(CMAKE_C_STANDARD 99)
project(cubec)
find_package(ICU REQUIRED COMPONENTS i18n uc data io)
include_directories(${PROJECT_SOURCE_DIR}/include)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/core SRC_CORE)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/ast SRC_AST)
add_executable(cubec ${SRC_CORE} ${SRC_AST} ${PROJECT_SOURCE_DIR}/src/cmd/cubec.c)
target_link_libraries(cubec PRIVATE 
    ICU::i18n 
    ICU::uc 
    ICU::data 
    ICU::io
)
