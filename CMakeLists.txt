cmake_minimum_required(VERSION 3.20)

project(CircuitSim VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CIRCUITSIM_BUILD_TESTS "Build CircuitSim tests" ON)

add_library(circuitsim
    src/netlist_parser.cpp
)

target_include_directories(circuitsim
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(circuitsim PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(circuitsim PRIVATE /W4 /permissive-)
else()
    target_compile_options(circuitsim PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CIRCUITSIM_BUILD_TESTS)
    enable_testing()

    add_executable(circuitsim_tests
        tests/test_main.cpp
        tests/netlist_parser_tests.cpp
    )

    target_link_libraries(circuitsim_tests PRIVATE circuitsim)
    target_include_directories(circuitsim_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

    if(MSVC)
        target_compile_options(circuitsim_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(circuitsim_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME circuitsim_tests COMMAND circuitsim_tests)
endif()
