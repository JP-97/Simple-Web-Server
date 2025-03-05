cmake_minimum_required(VERSION 3.16.3)

find_package(PkgConfig REQUIRED)
find_package(CMocka REQUIRED)

include_directories(${CMOCKA_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/tests/ut_include)
include_directories(${PROJECT_SOURCE_DIR}/include_public)
include_directories(${PROJECT_SOURCE_DIR}/include_private)

set(MOCKED_MODULES 
    "-Wl,--wrap=access"
    "-Wl,--wrap=fstat"
    "-Wl,--wrap=open")

function(ADD_SWS_TEST TEST_NAME)
    add_executable(${TEST_NAME} ut_src/${TEST_NAME}.c ut_src/mocks.c)
    add_dependencies(${TEST_NAME} ${TEST_NAME} libsws)
    add_test(${TEST_NAME} ${TEST_NAME})
    target_link_libraries(${TEST_NAME} libsws pthread cmocka ${LIBRARIES})
    target_link_options(${TEST_NAME} PRIVATE ${MOCKED_MODULES})
endfunction()

add_sws_test(test_http)
add_sws_test(test_command_line)
add_sws_test(test_bbuf)
add_sws_test(test_main)