cmake_minimum_required(VERSION 3.16.3)  # 3.16.3 is the max version supported by Jetson Nano
project(RealMQ C)

set(CMAKE_C_STANDARD 11)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-function")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g -OX") # Address Sanitizer

# Find ZeroMQ, CZMQ, JSON-C, libyaml, and Threads
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(ZMQ REQUIRED libzmq)
# Required for testing DGRAM sockets (UDP) that are not yet supported in stable releases only in draft releases
add_definitions(-DZMQ_BUILD_DRAFT_API)
#add_definitions(-DREALMQ_VERSION)       # This is the version of RealMQ with QoS support
#add_definitions(-DQOS_RETRANSMISSION)    # This is the version of RealMQ with QoS Retransmission support

pkg_check_modules(CZMQ REQUIRED libczmq)
pkg_check_modules(JSON-C REQUIRED json-c)
pkg_check_modules(YAML REQUIRED yaml-0.1)
pkg_check_modules(UUID REQUIRED uuid)

# Include all directories
include_directories(
        ${ZMQ_INCLUDE_DIRS}
        ${CZMQ_INCLUDE_DIRS}
        ${JSON-C_INCLUDE_DIRS}
        ${YAML_INCLUDE_DIRS}
        ${UUID_INCLUDE_DIRS}
        common
)


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
set(
        SOURCE_FILES

        # Common
        common/string_manip.c

        # Core
        common/core/config.c
        common/core/logger.c
        common/core/zhelpers.c

        # Qos
        common/qos/accrual_detector.c
        common/qos/message_queue.c
        common/qos/interpolation_search.c
        common/qos/dynamic_array.c
        common/qos/accrual_detector/heartbeat_history.c
        common/qos/accrual_detector/phi_accrual_failure_detector.c
        common/qos/accrual_detector/state.c

        # Utils
        common/utils/fs_utils.c
        common/utils/utils.c
        common/utils/time_utils.c
        common/utils/utils.c
        common/utils/memory_leak_detector.c
)


# Include dirs
include_directories(common)

# Link libraries to executables
function(target_link_libraries_realmq target)
    target_link_libraries(${target}
            ${ZMQ_LIBRARIES}
            ${CZMQ_LIBRARIES}
            ${JSON-C_LIBRARIES}
            ${YAML_LIBRARIES}
            ${UUID_LIBRARIES}
            Threads::Threads
            m # libm
    )
endfunction()

# Executables
add_executable(realmq_client src/realmq_client.c ${SOURCE_FILES})
add_executable(realmq_server src/realmq_server.c ${SOURCE_FILES})

target_link_libraries_realmq(realmq_client)
target_link_libraries_realmq(realmq_server)

# ------------------------------- Test executables ---------------------------------------
add_executable(client tests/draft_test/client.c ${SOURCE_FILES})
add_executable(server tests/draft_test/server.c ${SOURCE_FILES})

target_link_libraries_realmq(client)
target_link_libraries_realmq(server)

# ----------------------------------------------------------------------------------------

# ------------------------------- Unit Testing ---------------------------------------
# Include Unity source directory
include_directories(${CMAKE_CURRENT_BINARY_DIR}/unity-src/src)

# Enable testing for the project
enable_testing()

# Download and unpack Unity at configure time
configure_file(tests/unity/CMakeLists.txt unity-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/unity-download)
if (result)
    message(FATAL_ERROR "CMake step for unity failed: ${result}")
endif ()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/unity-download)
if (result)
    message(FATAL_ERROR "Build step for unity failed: ${result}")
endif ()

# Add Unity source to our build
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/unity-src
        ${CMAKE_CURRENT_BINARY_DIR}/unity-build
        EXCLUDE_FROM_ALL)

# Add the path to the Unity library to the link directories
link_directories(${CMAKE_CURRENT_BINARY_DIR}/unity-build)              #  fixme: probably not needed
link_directories(${CMAKE_CURRENT_BINARY_DIR}/unity-install/lib)


# Function to add Unity-based tests with less repetition
function(add_unity_test test_name test_source)
    add_executable(${test_name} ${test_source} ${SOURCE_FILES})
    target_link_libraries(${test_name} ${CMAKE_CURRENT_BINARY_DIR}/unity-install/lib/libunity.a)
    target_link_libraries_realmq(${test_name})
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# Test cases

add_unity_test(test_split_string tests/test_split_string.c)
add_unity_test(test_get_address tests/test_get_address.c)
add_unity_test(test_convert_string_to_int tests/test_convert_string_to_int.c)
add_unity_test(test_process_missed_message_ids tests/test_process_missed_message_ids.c)
add_unity_test(test_fs_utils tests/test_fs_utils.c)
add_unity_test(test_config tests/test_config.c)
add_unity_test(test_dynamic_array tests/test_dynamic_array.c)
# ----------------------------------------------------------------------------------------








