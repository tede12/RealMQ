cmake_minimum_required(VERSION 3.10)

project(unity-download NONE)

include(ExternalProject)

ExternalProject_Add(
        unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG master
        SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/unity-src"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/unity-build"
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/unity-install
        #        CONFIGURE_COMMAND ""
        #        BUILD_COMMAND ""
        #        INSTALL_COMMAND ""
        #        TEST_COMMAND ""
)