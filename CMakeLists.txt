cmake_minimum_required(VERSION 3.3)
project(ccml)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

FILE(GLOB FILE_LIB_CPP "source/lib/*.cpp")
FILE(GLOB FILE_LIB_H "source/lib/*.h")
add_library(ccml_lib ${FILE_LIB_CPP} ${FILE_LIB_H})

FILE(GLOB FILE_DRIVER_CPP "source/driver/*.cpp")
FILE(GLOB FILE_DRIVER_H "source/driver/*.h")
add_executable(ccml_driver ${FILE_DRIVER_CPP} ${FILE_DRIVER_H})
target_link_libraries(ccml_driver ccml_lib)

FILE(GLOB FILE_TESTS_CPP "source/tests/*.cpp")
FILE(GLOB FILE_TESTS_H "source/tests/*.h")
add_executable(ccml_tests  ${FILE_TESTS_CPP} ${FILE_TESTS_H})
target_link_libraries(ccml_tests ccml_lib)
