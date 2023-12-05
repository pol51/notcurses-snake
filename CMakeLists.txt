cmake_minimum_required(VERSION 3.9)

project(snake-nc)

set(CMAKE_CXX_STANDARD 17)

find_package(fmt REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_search_module(notcurses-core REQUIRED notcurses-core IMPORTED_TARGET)

file(GLOB snake_sources src/*.cpp src/*.h)
add_executable(${PROJECT_NAME} ${snake_sources})
target_link_libraries(
  ${PROJECT_NAME}
  PkgConfig::notcurses-core
  fmt::fmt
)
