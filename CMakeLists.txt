cmake_minimum_required(VERSION 3.25)
project(RealMQ C)

set(CMAKE_C_STANDARD 11)

# Find ZeroMQ
find_package(PkgConfig)

# Find and link ZeroMQ
pkg_check_modules(ZMQ REQUIRED libzmq)
include_directories(${ZMQ_INCLUDE_DIRS})

# Find and link CZMQ
pkg_check_modules(CZMQ "libczmq" REQUIRED)
include_directories(${CZMQ_INCLUDE_DIRS})

# Find and link JSON-C
pkg_check_modules(JSON-C REQUIRED json-c)
include_directories(${JSON-C_INCLUDE_DIRS})

# Find and link libyaml
pkg_check_modules(YAML REQUIRED yaml-0.1)
include_directories(${YAML_INCLUDE_DIRS})

# ZeroMQ, CZMQ, JSON-C, and libyaml library paths
if (APPLE)
    file(GLOB ZEROMQ_PATHS /opt/homebrew/Cellar/zeromq/*/lib)
    file(GLOB CZMQ_PATHS /opt/homebrew/Cellar/czmq/*/lib)
    file(GLOB JSON-C_PATHS /opt/homebrew/Cellar/json-c/*/lib)
    link_directories(${ZEROMQ_PATHS} ${CZMQ_PATHS} ${JSON-C_PATHS} ${YAML_LIBRARY_DIRS})
elseif (UNIX AND NOT APPLE)
    link_directories(${ZMQ_LIBRARY_DIRS} ${CZMQ_LIBRARY_DIRS} ${JSON-C_LIBRARY_DIRS} ${YAML_LIBRARY_DIRS})
endif ()

# More executables
add_executable(main main.c common/utils.h)

# client
add_executable(realmq_client realmq_client.c common/utils.h common/config.h)
target_link_libraries(realmq_client ${ZMQ_LIBRARIES} ${CZMQ_LIBRARIES} ${JSON-C_LIBRARIES} ${YAML_LIBRARIES})

# server
add_executable(realmq_server realmq_server.c common/utils.h common/config.h)
target_link_libraries(realmq_server ${ZMQ_LIBRARIES} ${CZMQ_LIBRARIES} ${JSON-C_LIBRARIES} ${YAML_LIBRARIES})

