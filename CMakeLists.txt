cmake_minimum_required(VERSION 3.20)

project(Nova VERSION 0.1.0 LANGUAGES CXX)

option(NOVA_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(nova_core
    src/lexer/token.cpp
    src/lexer/lexer.cpp
    src/ast/ast.cpp
    src/parser/parser.cpp
    src/semantic/analyzer.cpp
    src/codegen/cpp_generator.cpp
    src/compiler/compiler.cpp
    src/diagnostics/error.cpp
)

target_include_directories(nova_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(nova_core PUBLIC cxx_std_20)

add_executable(nova src/main.cpp)
target_link_libraries(nova PRIVATE nova_core)
target_compile_definitions(nova PRIVATE NOVA_VERSION="${PROJECT_VERSION}")

if(MSVC)
    target_compile_options(nova_core PRIVATE /W4 /permissive-)
    target_compile_options(nova PRIVATE /W4 /permissive-)
    if(NOVA_WARNINGS_AS_ERRORS)
        target_compile_options(nova_core PRIVATE /WX)
        target_compile_options(nova PRIVATE /WX)
    endif()
else()
    foreach(target nova_core nova)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        if(NOVA_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endforeach()
endif()
