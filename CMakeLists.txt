cmake_minimum_required(VERSION 3.20)

project(CircuitSim VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CIRCUITSIM_BUILD_TESTS "Build CircuitSim tests" ON)

add_library(circuitsim
    src/dc_solver.cpp
    src/netlist_parser.cpp
    src/transient_solver.cpp
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

add_executable(circuitsim_cli
    src/main.cpp
)

target_link_libraries(circuitsim_cli PRIVATE circuitsim)

if(MSVC)
    target_compile_options(circuitsim_cli PRIVATE /W4 /permissive-)
else()
    target_compile_options(circuitsim_cli PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CIRCUITSIM_BUILD_TESTS)
    enable_testing()

    add_executable(circuitsim_tests
        tests/dc_solver_tests.cpp
        tests/test_main.cpp
        tests/netlist_parser_tests.cpp
        tests/transient_solver_tests.cpp
    )

    target_link_libraries(circuitsim_tests PRIVATE circuitsim)
    target_include_directories(circuitsim_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

    if(MSVC)
        target_compile_options(circuitsim_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(circuitsim_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME circuitsim_tests COMMAND circuitsim_tests)
    add_test(
        NAME circuitsim_cli_resistor_divider
        COMMAND circuitsim_cli ${PROJECT_SOURCE_DIR}/examples/resistor_divider.cir
    )
    set_tests_properties(
        circuitsim_cli_resistor_divider
        PROPERTIES
        PASS_REGULAR_EXPRESSION "out = 3.333333"
    )
    add_test(
        NAME circuitsim_cli_rc_charge
        COMMAND circuitsim_cli tran ${PROJECT_SOURCE_DIR}/examples/rc_charge.cir 1e-4 5e-3
    )
    set_tests_properties(
        circuitsim_cli_rc_charge
        PROPERTIES
        PASS_REGULAR_EXPRESSION "0\\.005000,0\\.000000,5\\.000000,4\\."
    )
endif()
