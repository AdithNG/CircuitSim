cmake_minimum_required(VERSION 3.20)

project(CircuitSim VERSION 0.1.0 LANGUAGES CXX)

find_package(Python3 COMPONENTS Interpreter Development.Module)

include(FetchContent)

FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.13.6
)
FetchContent_MakeAvailable(pybind11)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CIRCUITSIM_BUILD_TESTS "Build CircuitSim tests" ON)

add_library(circuitsim
    src/diagnostics.cpp
    src/dc_solver.cpp
    src/netlist_parser.cpp
    src/transient_solver.cpp
)

set_target_properties(circuitsim PROPERTIES POSITION_INDEPENDENT_CODE ON)

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

if(Python3_Development.Module_FOUND)
    pybind11_add_module(circuitsim_py python/bindings.cpp)
    target_link_libraries(circuitsim_py PRIVATE circuitsim)

    set_target_properties(
        circuitsim_py
        PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/python
        LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/python
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/python
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PROJECT_BINARY_DIR}/python
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${PROJECT_BINARY_DIR}/python
    )
endif()

if(CIRCUITSIM_BUILD_TESTS)
    enable_testing()

    add_executable(circuitsim_tests
        tests/diagnostics_tests.cpp
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
    if(Python3_Interpreter_FOUND)
        add_test(
            NAME circuitsim_plot_transient_smoke
            COMMAND
                ${Python3_EXECUTABLE}
                ${PROJECT_SOURCE_DIR}/tests/plot_transient_smoke.py
                --cli
                $<TARGET_FILE:circuitsim_cli>
        )
        set_tests_properties(
            circuitsim_plot_transient_smoke
            PROPERTIES
            PASS_REGULAR_EXPRESSION "Wrote plot to"
        )
    endif()
    if(Python3_Interpreter_FOUND AND Python3_Development.Module_FOUND)
        add_test(
            NAME circuitsim_python_bindings_smoke
            COMMAND
                ${Python3_EXECUTABLE}
                ${PROJECT_SOURCE_DIR}/tests/python_bindings_smoke.py
                --module-dir
                ${PROJECT_BINARY_DIR}/python
        )
        set_tests_properties(
            circuitsim_python_bindings_smoke
            PROPERTIES
            PASS_REGULAR_EXPRESSION "Python bindings smoke test passed"
        )
        add_test(
            NAME circuitsim_parameter_sweep_smoke
            COMMAND
                ${Python3_EXECUTABLE}
                ${PROJECT_SOURCE_DIR}/tests/parameter_sweep_smoke.py
                --module-dir
                ${PROJECT_BINARY_DIR}/python
        )
        set_tests_properties(
            circuitsim_parameter_sweep_smoke
            PROPERTIES
            PASS_REGULAR_EXPRESSION "Parameter sweep smoke test passed"
        )
        add_test(
            NAME circuitsim_chip_showcase_smoke
            COMMAND
                ${Python3_EXECUTABLE}
                ${PROJECT_SOURCE_DIR}/tests/chip_showcase_smoke.py
                --module-dir
                ${PROJECT_BINARY_DIR}/python
        )
        set_tests_properties(
            circuitsim_chip_showcase_smoke
            PROPERTIES
            PASS_REGULAR_EXPRESSION "Chip showcase smoke test passed"
        )
    endif()
endif()
