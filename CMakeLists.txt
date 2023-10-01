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
find_package(PkgConfig)
pkg_check_modules(JSON-C REQUIRED json-c)
include_directories(${JSON-C_INCLUDE_DIRS})

# ZeroMQ, CZMQ, and JSON-C library paths
if (APPLE)
    # Homebrew installation, glob is needed because of different version numbers
    file(GLOB ZEROMQ_PATHS /opt/homebrew/Cellar/zeromq/*/lib)
    file(GLOB CZMQ_PATHS /opt/homebrew/Cellar/czmq/*/lib)
    file(GLOB JSON-C_PATHS /opt/homebrew/Cellar/json-c/*/lib)
    link_directories(${ZEROMQ_PATHS} ${CZMQ_PATHS} ${JSON-C_PATHS})
elseif (UNIX AND NOT APPLE)
    link_directories(${ZMQ_LIBRARY_DIRS} ${CZMQ_LIBRARY_DIRS} ${JSON-C_LIBRARY_DIRS})
endif ()


# More executables

# client
add_executable(realmq_client realmq_client.c)
target_link_libraries(realmq_client ${ZMQ_LIBRARIES} ${CZMQ_LIBRARIES} ${JSON-C_LIBRARIES})

# server
add_executable(realmq_server realmq_server.c)
target_link_libraries(realmq_server ${ZMQ_LIBRARIES} ${CZMQ_LIBRARIES} ${JSON-C_LIBRARIES})

