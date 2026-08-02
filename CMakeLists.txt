cmake_minimum_required(VERSION 3.16)

project(J1939)

set(CMAKE_CXX_STANDARD 17)

# Standard macro to build a STATIC library.
macro(define_lib LIB_NAME)
    cmake_minimum_required(VERSION 3.10)
    project (${LIB_NAME})
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Werror -Wall -Wextra -Wno-unused-parameter")
    set (CMAKE_INCLUDE_CURRENT_DIR ON)
    file(GLOB SRCS "*.cpp")
    file(GLOB HDRS "*.h")
    list(APPEND PUBLIC_HEADERS ${HDRS})
    set (PUBLIC_HEADERS  ${PUBLIC_HEADERS} PARENT_SCOPE)
    add_library(${LIB_NAME} SHARED ${HDRS} ${SRCS} ${ARGN})
endmacro()

# List all subfolders which generate static libs
list(APPEND TARGETS
        j1939_parser
        can_stream
        can_reader)

include_directories(options)
include_directories(j1939_parser/dbc_parser)

foreach(target ${TARGETS})
    include_directories(${target})
endforeach()

foreach(target ${TARGETS})
    add_subdirectory(${target})
endforeach()

include_directories(can_sandbox_lib)
add_subdirectory(can_sandbox_lib)

# Console application
add_subdirectory(console)

# DBC compiler
add_executable(dbc_compiler dbc_compiler/dbc_compiler.cpp)
target_link_libraries(dbc_compiler j1939_parser)
install(TARGETS dbc_compiler RUNTIME DESTINATION bin)

# Enable unit tests in a standalone build or when UNIT_TEST was added to the bitbake recipe.
find_package(GTest)

add_definitions(-DDBC_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}/tests/dbc/")

# Build test app
file(GLOB ALL_TESTS "tests/*cpp")
add_executable(all_tests ${ALL_TESTS})
add_test(all_tests ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/all_tests)

# Tell linker about all libs to be used there
target_link_libraries(all_tests GTest::GTest GTest::Main j1939_parser)
# Make it discoverable
gtest_discover_tests(all_tests)
