cmake_minimum_required(VERSION 3.16.3)

project(sws)

set(SOURCES
    src/main.c
    src/command_line.c
    src/rio.c
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(sws PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Install target to /usr/local/bin/ so it's executable by name
install(TARGETS sws DESTINATION bin)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
