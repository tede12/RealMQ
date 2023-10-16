cmake_minimum_required(VERSION 3.16.3)  # 3.16.3 is the max version supported by Jetson Nano
project(RealMQ C)

set(CMAKE_C_STANDARD 11)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-function")

# Find ZeroMQ, CZMQ, JSON-C, libyaml, and Threads
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(ZMQ REQUIRED libzmq)
# Required for testing DGRAM sockets (UDP) that are not yet supported in stable releases only in draft releases
add_definitions(-DZMQ_BUILD_DRAFT_API)
add_definitions(-DREALMQ_VERSION)   # This is the version of RealMQ with QoS support

pkg_check_modules(CZMQ REQUIRED libczmq)
pkg_check_modules(JSON-C REQUIRED json-c)
pkg_check_modules(YAML REQUIRED yaml-0.1)

# Include all directories
include_directories(${ZMQ_INCLUDE_DIRS} ${CZMQ_INCLUDE_DIRS} ${JSON-C_INCLUDE_DIRS} ${YAML_INCLUDE_DIRS} common)


# ZeroMQ, CZMQ, JSON-C, and libyaml library paths
if (APPLE)
    set(ZEROMQ_CUSTOM_PATH "~/libzmq/src/.libs") # This is where the compiled version of libzmq is located (with draft support enabled)

    include_directories(${ZEROMQ_CUSTOM_PATH})
    link_directories(${ZEROMQ_CUSTOM_PATH})

    file(GLOB CZMQ_PATHS /opt/homebrew/Cellar/czmq/*/lib)
    file(GLOB JSON-C_PATHS /opt/homebrew/Cellar/json-c/*/lib)
    link_directories(${CZMQ_PATHS} ${JSON-C_PATHS} ${YAML_LIBRARY_DIRS})
elseif (UNIX AND NOT APPLE)
    link_directories(${ZMQ_LIBRARY_DIRS} ${CZMQ_LIBRARY_DIRS} ${JSON-C_LIBRARY_DIRS} ${YAML_LIBRARY_DIRS})
endif ()

# Source files that need to be compiled
set(SOURCE_FILES
        common/qos.c
        common/logger.c
        common/message_queue.c
        common/config.c
        common/utils.c
        common/zhelpers.c
        )


# Include common files
include_directories(common)

# Executables
add_executable(realmq_client realmq_client.c ${SOURCE_FILES})
add_executable(realmq_server realmq_server.c ${SOURCE_FILES})

# ------------------------------- Test executables ---------------------------------------
add_executable(client draft_test/client.c ${SOURCE_FILES})
add_executable(server draft_test/server.c ${SOURCE_FILES})
target_link_libraries(client ${ZMQ_LIBRARIES} ${CZMQ_LIBRARIES} ${JSON-C_LIBRARIES} ${YAML_LIBRARIES} Threads::Threads)
target_link_libraries(server ${ZMQ_LIBRARIES} ${CZMQ_LIBRARIES} ${JSON-C_LIBRARIES} ${YAML_LIBRARIES} Threads::Threads)
# ----------------------------------------------------------------------------------------

target_link_libraries(realmq_client
        ${ZMQ_LIBRARIES}
        ${CZMQ_LIBRARIES}
        ${JSON-C_LIBRARIES}
        ${YAML_LIBRARIES}
        Threads::Threads)

target_link_libraries(realmq_server
        ${ZMQ_LIBRARIES}
        ${CZMQ_LIBRARIES}
        ${JSON-C_LIBRARIES}
        ${YAML_LIBRARIES}
        Threads::Threads)