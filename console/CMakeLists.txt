cmake_minimum_required(VERSION 3.16)
project(can_sandbox)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CURRENT_SOURCE_DIR on)

add_executable(can_sandbox can_sandbox.cpp)
add_dependencies(can_sandbox can_sandbox_lib j1939_parser can_reader can_streams)

target_link_libraries(can_sandbox can_sandbox_lib j1939_parser can_reader can_streams ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS can_sandbox RUNTIME DESTINATION bin)
