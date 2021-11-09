cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(risc-v-sim VERSION 1.0 LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)

# Move the resulting binary to ./build/<bin>
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

add_subdirectory(src)

target_include_directories(risc-v-sim PUBLIC
    "${PUBLIC_BINARY_DIR}"
    "${PUBLIC_SOURCE_DIR}"
    )
