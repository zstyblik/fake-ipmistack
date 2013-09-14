cmake_minimum_required(VERSION 2.6)
project(fake-ipmistack)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c99 -Werror -pedantic -Wformat -Wformat-nonliteral")
add_subdirectory(lib)
add_subdirectory(src)
include_directories(${CMAKE_SOURCE_DIR}/include)
