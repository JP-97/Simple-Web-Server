cmake_minimum_required(VERSION 3.16.3)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

project(sws)

set(SOURCES
    src/main.c
    src/command_line.c
    src/rio.c
    src/http.c
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(sws PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Install target to /usr/local/bin/ so it's executable by name
install(TARGETS sws DESTINATION bin)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

include(AddCMockaTest)
if (UNIT_TESTING)
    add_subdirectory(tests)
endif (UNIT_TESTING)
