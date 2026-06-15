cmake_minimum_required(VERSION 3.16)

project(MiniLangVM LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(minilang
    src/main.cpp
    src/lexer.cpp
    src/ast.cpp
    src/ast_printer.cpp
    src/asm_codegen.cpp
    src/parser.cpp
    src/semantic.cpp
    src/bytecode.cpp
    src/codegen.cpp
    src/vm.cpp
)

target_include_directories(minilang PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(minilang PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# Required project targets


# Build target required by the assignment.
# Usage:
#   cmake --build build --target build
add_custom_target(build
    DEPENDS minilang
    COMMENT "Building MiniLangVM compiler"
)

# Run target required by the assignment.
# Usage:
#   cmake --build build --target run
add_custom_target(run
    COMMAND $<TARGET_FILE:minilang> ${CMAKE_CURRENT_SOURCE_DIR}/examples/run_hello.ml
    DEPENDS minilang
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running MiniLangVM example program"
)

# Debug target required by the assignment.
# It creates a separate Debug build directory and builds the project there.
# Usage:
#   cmake --build build --target debug
add_custom_target(debug
    COMMAND ${CMAKE_COMMAND}
            -S ${CMAKE_CURRENT_SOURCE_DIR}
            -B ${CMAKE_CURRENT_SOURCE_DIR}/build-debug
            -DCMAKE_BUILD_TYPE=Debug
    COMMAND ${CMAKE_COMMAND}
            --build ${CMAKE_CURRENT_SOURCE_DIR}/build-debug
    COMMENT "Configuring and building Debug version"
)

# Clean target:
# CMake already provides the built-in target:
#   cmake --build build --target clean

# Run all valid and invalid language tests.
# Usage:
#   cmake --build build --target test_all
add_custom_target(test_all
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.sh
    DEPENDS minilang
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running MiniLangVM valid and invalid tests"
)

# Generate, assemble, link and run a small x86-64 backend example.
# Usage:
#   cmake --build build --target asm_example
find_program(NASM_EXECUTABLE nasm)

if (NASM_EXECUTABLE)
    add_custom_target(asm_example
        COMMAND $<TARGET_FILE:minilang>
                ${CMAKE_CURRENT_SOURCE_DIR}/examples/asm_print_multi.ml
                --emit-asm
                -o ${CMAKE_BINARY_DIR}/asm_print_multi.asm

        COMMAND ${NASM_EXECUTABLE}
                -f elf64
                ${CMAKE_BINARY_DIR}/asm_print_multi.asm
                -o ${CMAKE_BINARY_DIR}/asm_print_multi.o

        COMMAND ${CMAKE_CXX_COMPILER}
                -no-pie
                ${CMAKE_BINARY_DIR}/asm_print_multi.o
                -o ${CMAKE_BINARY_DIR}/asm_print_multi

        COMMAND ${CMAKE_BINARY_DIR}/asm_print_multi

        DEPENDS minilang
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating, assembling, linking and running x86-64 asm example"
    )
else()
    add_custom_target(asm_example
        COMMAND ${CMAKE_COMMAND} -E echo "nasm was not found. Install it with: sudo apt install nasm"
        COMMENT "NASM is required for asm_example"
    )
endif()

# Build x86-64 executable from a MiniLang example through the asm backend.
# Usage:
#   cmake --build build --target asm_array
add_custom_target(asm_array
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_asm.sh
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/asm_demo.ml
            ${CMAKE_BINARY_DIR}/asm_array
    DEPENDS minilang
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building x86-64 executable from MiniLang array example"
)

# Build and run x86-64 executable from a MiniLang example through the asm backend.
# Usage:
#   cmake --build build --target run_asm_array
add_custom_target(run_asm_array
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_asm.sh
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/asm_demo.ml
            ${CMAKE_BINARY_DIR}/asm_array
    DEPENDS minilang
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building and running x86-64 executable from MiniLang array example"
)

# Run x86-64 asm backend tests.
# Usage:
#   cmake --build build --target asm_tests
add_custom_target(asm_tests
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_asm.sh
    DEPENDS minilang
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running x86-64 asm backend tests"
)


# Run full project verification:
# - configure/build compiler
# - run VM backend tests
# - run x86-64 asm backend tests
# Usage:
#   cmake --build build --target verify_all
add_custom_target(verify_all
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_project.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running full MiniLangVM project verification"
)

# Golden tests for MiniLangVM.
# Runs programs from tests/golden and compares stdout/stderr/exit code
# with .stdout/.stderr/.code golden files.
enable_testing()

add_custom_target(golden
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/run_golden_tests.py
            --minilang ${CMAKE_BINARY_DIR}/minilang
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS minilang
)

add_test(
    NAME golden
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/run_golden_tests.py
            --minilang ${CMAKE_BINARY_DIR}/minilang
)

# Make ctest run golden tests from project root,
# because tests/golden is located in the source directory.
set_tests_properties(golden PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
