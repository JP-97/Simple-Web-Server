cmake_minimum_required(VERSION 3.16.3)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

project(sws)

set(SOURCES
    src/main.c
    src/command_line.c
    src/rio.c
    src/http.c
    src/bbuf.c
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic")

add_executable(${PROJECT_NAME} ${SOURCES})
add_library(libsws ${SOURCES})

target_include_directories(sws PRIVATE include_public)
target_include_directories(sws PRIVATE include_private)
target_include_directories(libsws PRIVATE include_public)
target_include_directories(libsws PRIVATE include_private)


set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

target_link_libraries(sws pthread)
target_link_libraries(libsws pthread)


# Add Unit Tests
enable_testing()
add_subdirectory(tests)

