find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

FILE(GLOB FILE_IDE_SRC "imgui/*.cpp" "imgui/*.h")
add_executable(nano_ide ${FILE_IDE_SRC} "main.cpp")
target_link_libraries(nano_ide
  nano_lib_common
  nano_lib_compiler
  nano_lib_vm
  nano_lib_builtin
  ${SDL2_LIBRARIES}
  OPENGL32.lib)
