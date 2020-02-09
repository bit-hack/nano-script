cmake_minimum_required(VERSION 3.3)
project(ccml)

set(CCML_STRICTCOMPILER false CACHE BOOL "Enable strict compiler flags")

if (CCML_STRICTCOMPILER)
  if(MSVC)
    add_compile_options("/W4" "/WX")
  else()
    add_compile_options("-std=c++11" "-Wall" "-Wextra" "-pedantic" "-Werror" "-Wno-error=nonnull-compare")
  endif()
endif()

FILE(GLOB FILE_LIB_CPP "source/lib/*.cpp")
FILE(GLOB FILE_LIB_H "source/lib/*.h")
add_library(ccml_lib ${FILE_LIB_CPP} ${FILE_LIB_H})

FILE(GLOB FILE_DRIVER_CPP "source/driver/*.cpp")
FILE(GLOB FILE_DRIVER_H "source/driver/*.h")
add_executable(ccml_driver ${FILE_DRIVER_CPP} ${FILE_DRIVER_H})
target_link_libraries(ccml_driver ccml_lib)

FILE(GLOB FILE_COMP_CPP "source/comp/*.cpp")
FILE(GLOB FILE_COMP_H "source/comp/*.h")
add_executable(ccml_comp ${FILE_COMP_CPP} ${FILE_COMP_H})
target_link_libraries(ccml_comp ccml_lib)

set(CCML_BUILD_SDL_DRIVER false CACHE BOOL "Build the SDL driver")

if (${CCML_BUILD_SDL_DRIVER})
  find_package(SDL REQUIRED)
  include_directories(${SDL2_INCLUDE_DIR})
  FILE(GLOB FILE_DRIVER_SDL_CPP "source/driver_sdl/*.cpp")
  FILE(GLOB FILE_DRIVER_SDL_H "source/driver_sdl/*.h")
  add_executable(ccml_driver_sdl ${FILE_DRIVER_SDL_CPP} ${FILE_DRIVER_SDL_H})
  target_link_libraries(ccml_driver_sdl ccml_lib ${SDL_LIBRARY_TEMP})
endif()
