
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

FILE(GLOB FILE_IDE_SRC "imgui/*.cpp" "imgui/*.h")
add_executable(ccml_ide ${FILE_IDE_SRC} "main.cpp")
target_link_libraries(ccml_ide
  ccml_lib_common
  ccml_lib_compiler
  ccml_lib_vm
  ccml_lib_builtin
  ${SDL2_LIBRARIES}
  OPENGL32.lib)
