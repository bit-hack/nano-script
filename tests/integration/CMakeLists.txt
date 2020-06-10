find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(nano_integration_tests "main.cpp")
target_link_libraries(
  nano_integration_tests
  nano_lib_common
  nano_lib_compiler
  nano_lib_vm
  nano_lib_builtin
  ${SDL2_LIBRARIES}
  OPENGL32.lib)
