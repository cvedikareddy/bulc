cmake_minimum_required(VERSION 3.16)
project(bulc_rule_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
add_compile_options(-Wall -Wextra)

# Compiler library: grows as each pipeline stage is added.
add_library(bulc_compiler STATIC
  compiler/lexer/lexer.cpp
  compiler/parser/parser.cpp
)
target_include_directories(bulc_compiler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

enable_testing()
add_executable(frontend_test tests/parser_tests/frontend_test.cpp)
target_link_libraries(frontend_test bulc_compiler)
add_test(NAME frontend_test
         COMMAND frontend_test ${CMAKE_CURRENT_SOURCE_DIR}/examples/fraud_rules.rule)