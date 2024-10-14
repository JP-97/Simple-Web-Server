cmake_minimum_required(VERSION 3.16.3)

find_package(PkgConfig REQUIRED)
find_package(CMocka REQUIRED)

include_directories(${CMOCKA_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/include)

function(ADD_SWS_TEST TEST_NAME)
    add_executable(${TEST_NAME} ut_src/${TEST_NAME}.c)
    add_dependencies(${TEST_NAME} ${TEST_NAME} libsws)
    add_test(${TEST_NAME} ${TEST_NAME}) # TODO figure out how ctest works...
    target_link_libraries(${TEST_NAME} libsws pthread cmocka ${LIBRARIES})
endfunction()

add_sws_test(test_command_line)
add_sws_test(test_bbuf)
add_sws_test(test_main)
