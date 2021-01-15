cmake_minimum_required(VERSION 3.8)

project(leptonSDK C CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

file(GLOB srcs "*.c")
file(GLOB hdrs "*.h")

add_library(${PROJECT_NAME} ${srcs})

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(FILES ${hdrs} DESTINATION include)
