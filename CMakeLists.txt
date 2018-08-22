cmake_minimum_required(VERSION 3.3)
project(ccml)

FILE(GLOB FILE_CPP "source/*.cpp")
FILE(GLOB FILE_H "source/*.h")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(ccml ${FILE_CPP} ${FILE_H})
