cmake_minimum_required(VERSION 3.16.3) # o la versione che stai attualmente utilizzando
project(RealMQ C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-function")

# Trova ZeroMQ e Threads
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)


pkg_check_modules(ZMQ REQUIRED libzmq)
# Necessario per testare i socket DGRAM (UDP) che non sono ancora supportati nelle release stabili ma solo in quelle draft
add_definitions(-DZMQ_BUILD_DRAFT_API)

# Include le directory
include_directories(${ZMQ_INCLUDE_DIRS} common)

# Percorsi delle librerie ZeroMQ
if (APPLE)
    # Aggiungi il percorso delle intestazioni qui
    set(ZEROMQ_CUSTOM_INCLUDE_DIR "~/libzmq/include")
    set(ZEROMQ_CUSTOM_LIBRARY "~/libzmq/src/.libs/libzmq.a")
    set(ENV{DYLD_LIBRARY_PATH} "~/libzmq/src/.libs:$ENV{DYLD_LIBRARY_PATH}")

    include_directories(~/libzmq/include)

    # Specifica il percorso della tua build di ZeroMQ qui
    link_directories(~/libzmq/src/.libs)
elseif (UNIX AND NOT APPLE)
    link_directories(${ZMQ_LIBRARY_DIRS})
endif ()

# File sorgente che devono essere compilati
set(SERVER_SOURCE_FILES server.c)
set(CLIENT_SOURCE_FILES client.c)

# Eseguibili
add_executable(server ${SERVER_SOURCE_FILES})
add_executable(client ${CLIENT_SOURCE_FILES})

# Collega le librerie
target_link_libraries(server ${ZMQ_LIBRARIES} Threads::Threads)
target_link_libraries(client ${ZMQ_LIBRARIES} Threads::Threads)
