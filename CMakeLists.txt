cmake_minimum_required(VERSION 3.20)

project(SSIcon)

set(CMAKE_CXX_STANDARD 20)

add_executable(SSIcon WIN32 main.cpp)

target_link_libraries(SSIcon
user32 shell32
)