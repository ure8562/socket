cmake_minimum_required(VERSION 3.0.0)
project(Socket-client VERSION 0.1.0)

include(CTest)
enable_testing()

#header include 경로
#include_directories(./include)

add_executable(client socket-client.cpp)

#
TARGET_LINK_LIBRARIES(client -lpthread)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
