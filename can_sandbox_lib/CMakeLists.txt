define_lib(can_sandbox_lib)

add_dependencies(can_sandbox_lib can_streams can_reader)

target_link_libraries(can_sandbox_lib can_streams ${CMAKE_THREAD_LIBS_INIT})
