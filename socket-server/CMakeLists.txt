cmake_minimum_required(VERSION 3.0.0)
project(Socket-Server VERSION 0.1.0)

include(CTest)
enable_testing()

#header include 경로
#include_directories(./mariadb_work)

#add_subdirectory(mariadb_work)
add_executable(server socket-server.cpp ./mariadb_work/mariadb_work.cpp)

#TARGET_LINK_LIBRARIES(server PUBLIC -lpthread -lmysqlclient Mariadb_work)
TARGET_LINK_LIBRARIES(server PUBLIC -lpthread -lmysqlclient)

#LINK_LIBRARIES(-lmysqlclient)



set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
