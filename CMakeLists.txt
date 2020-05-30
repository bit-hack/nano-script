cmake_minimum_required(VERSION 3.3)
project(nano)

set(NANO_STRICTCOMPILER false CACHE BOOL "Enable strict compiler flags")

add_definitions("-D_CRT_SECURE_NO_WARNINGS")

if (NANO_STRICTCOMPILER)
  if(MSVC)
    add_compile_options("/W4" "/WX")
  else()
    add_compile_options("-std=c++11" "-Wall" "-Wextra" "-pedantic" "-Werror" "-Wno-error=nonnull-compare")
  endif()
endif()

FILE(GLOB FILE_LIB_COMMON_CPP "source/lib_common/*.cpp")
FILE(GLOB FILE_LIB_COMMON_H "source/lib_common/*.h")
add_library(nano_lib_common ${FILE_LIB_COMMON_CPP} ${FILE_LIB_COMMON_H})

include_directories("source/lib_common")

FILE(GLOB FILE_LIB_COMPILER_CPP "source/lib_compiler/*.cpp")
FILE(GLOB FILE_LIB_COMPILER_H "source/lib_compiler/*.h")
add_library(nano_lib_compiler ${FILE_LIB_COMPILER_CPP} ${FILE_LIB_COMPILER_H})

FILE(GLOB FILE_LIB_VM_CPP "source/lib_vm/*.cpp")
FILE(GLOB FILE_LIB_VM_H "source/lib_vm/*.h")
add_library(nano_lib_vm ${FILE_LIB_VM_CPP} ${FILE_LIB_VM_H})

FILE(GLOB FILE_LIB_BUILTIN_CPP "source/lib_builtins/*.cpp")
FILE(GLOB FILE_LIB_BUILTIN_H "source/lib_builtins/*.h")
add_library(nano_lib_builtin ${FILE_LIB_BUILTIN_CPP} ${FILE_LIB_BUILTIN_H})

set(NANO_BUILD_DRIVER false CACHE BOOL "Build the driver")
set(NANO_BUILD_SDL_DRIVER false CACHE BOOL "Build the SDL driver")
set(NANO_BUILD_COMP false CACHE BOOL "Build the compiler")
set(NANO_BUILD_IDE false CACHE BOOL "Build the IDE")

if (${NANO_BUILD_IDE} OR ${NANO_BUILD_SDL_DRIVER})
  find_package(SDL2 REQUIRED)
  message(STATUS "SDL include ${SDL2_INCLUDE_DIRS}")
  message(STATUS "SDL library ${SDL2_LIBRARIES}")
endif()

if (${NANO_BUILD_DRIVER})
  FILE(GLOB FILE_DRIVER_CPP "source/driver/*.cpp")
  FILE(GLOB FILE_DRIVER_H "source/driver/*.h")
  add_executable(nano_driver ${FILE_DRIVER_CPP} ${FILE_DRIVER_H})
  target_link_libraries(nano_driver nano_lib_common nano_lib_compiler nano_lib_vm nano_lib_builtin)
endif()

if (${NANO_BUILD_COMP})
  FILE(GLOB FILE_COMP_CPP "source/comp/*.cpp")
  FILE(GLOB FILE_COMP_H "source/comp/*.h")
  add_executable(nano_comp ${FILE_COMP_CPP} ${FILE_COMP_H})
  target_link_libraries(nano_comp nano_lib_common nano_lib_compiler nano_lib_builtin)
endif()

if (${NANO_BUILD_SDL_DRIVER})
  include_directories(${SDL2_INCLUDE_DIRS})
  FILE(GLOB FILE_DRIVER_SDL_CPP "source/driver_sdl/*.cpp")
  FILE(GLOB FILE_DRIVER_SDL_H "source/driver_sdl/*.h")
  add_executable(nano_driver_sdl ${FILE_DRIVER_SDL_CPP} ${FILE_DRIVER_SDL_H})
  target_link_libraries(nano_driver_sdl nano_lib_common nano_lib_compiler nano_lib_vm nano_lib_builtin ${SDL2_LIBRARIES})
endif()

if (${NANO_BUILD_IDE})
  add_subdirectory("examples/ide")
endif()
