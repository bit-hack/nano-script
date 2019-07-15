cmake_minimum_required(VERSION 3.3)
project(ccml)

find_package(SDL REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

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

FILE(GLOB FILE_DRIVER_SDL_CPP "source/driver_sdl/*.cpp")
FILE(GLOB FILE_DRIVER_SDL_H "source/driver_sdl/*.h")
add_executable(ccml_driver_sdl ${FILE_DRIVER_SDL_CPP} ${FILE_DRIVER_SDL_H})
target_link_libraries(ccml_driver_sdl ccml_lib ${SDL_LIBRARY_TEMP})

FILE(GLOB FILE_TESTS_CPP "source/tests/*.cpp")
FILE(GLOB FILE_TESTS_H "source/tests/*.h")
add_executable(ccml_tests  ${FILE_TESTS_CPP} ${FILE_TESTS_H})
target_link_libraries(ccml_tests ccml_lib)

FILE(GLOB FILE_FUZZ_CPP "source/fuzz/*.cpp")
FILE(GLOB FILE_FUZZ_H "source/fuzz/*.h")
add_executable(ccml_fuzz  ${FILE_FUZZ_CPP} ${FILE_FUZZ_H})
target_link_libraries(ccml_fuzz ccml_lib)

