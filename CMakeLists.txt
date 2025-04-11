cmake_minimum_required(VERSION 3.16)
project(fretwork VERSION 0.1.0 LANGUAGES CXX)

add_library(fretwork INTERFACE)
add_library(fretwork::fretwork ALIAS fretwork)
target_include_directories(fretwork INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_features(fretwork INTERFACE cxx_std_20)

option(FRETWORK_BUILD_TESTS "Build the test suite" ON)
option(FRETWORK_BUILD_EXAMPLES "Build the examples" ON)

if(FRETWORK_BUILD_TESTS)
    enable_testing()
    add_executable(fretwork_tests
        tests/main.cpp
        tests/test_expected.cpp
        tests/test_optional.cpp
        tests/test_format.cpp
        tests/test_strings.cpp)
    target_link_libraries(fretwork_tests PRIVATE fretwork)
    target_compile_options(fretwork_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME fretwork_tests COMMAND fretwork_tests)
endif()

if(FRETWORK_BUILD_EXAMPLES)
    add_executable(tour examples/tour.cpp)
    target_link_libraries(tour PRIVATE fretwork)
    target_compile_options(tour PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

install(DIRECTORY include/ DESTINATION include)
