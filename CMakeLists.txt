cmake_minimum_required(VERSION 3.16.3)  # 3.16.3 is the max version supported by Jetson Nano
project(RealMQ C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_STANDARD 17)

# Find ZeroMQ, CZMQ, JSON-C, libyaml, and Threads
find_package(PkgConfig)
find_package(Threads REQUIRED)

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
#    # Threads configuration (https://stackoverflow.com/questions/34143265/undefined-reference-to-symbol-pthread-createglibc-2-2-5)
#    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
#    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
#    set(CMAKE_HAVE_THREADS_LIBRARY 1)
#    set(CMAKE_USE_WIN32_THREADS_INIT 0)
#    set(CMAKE_USE_PTHREADS_INIT 1)
#    set(THREADS_PREFER_PTHREAD_FLAG ON)
endif ()


# client
add_executable(realmq_client realmq_client.c common/utils.h common/config.h common/logger.h)
target_link_libraries(realmq_client ${ZMQ_LIBRARIES} ${CZMQ_LIBRARIES} ${JSON-C_LIBRARIES} ${YAML_LIBRARIES} Threads::Threads)

# server
add_executable(realmq_server realmq_server.c common/utils.h common/config.h common/logger.h)
target_link_libraries(realmq_server ${ZMQ_LIBRARIES} ${CZMQ_LIBRARIES} ${JSON-C_LIBRARIES} ${YAML_LIBRARIES} Threads::Threads)

add_executable(thread_test main.c)
target_link_libraries(thread_test Threads::Threads)