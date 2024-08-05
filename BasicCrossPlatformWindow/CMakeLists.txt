cmake_minimum_required(VERSION 3.10)
project(BasicCrossPlatformWindow)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDART_REQUIRED True)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -h")

include_directories(src)

set(SOURCES
  src/main.c
)

add_executable(PLACEHOLDER ${SOURCES})

if(UNIX)
  find_package(X11 REQUIRED)
  target_link_libraries(PLACEHOLDER X11 pthread m)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_INTERMEDIATE_DIR ${CMAKE_BINARY_DIR}/intermediate)
