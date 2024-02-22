cmake_minimum_required(VERSION 3.8)
project(moscc)


cmake_policy(SET CMP0057 NEW) # Enable IN_LIST operator
cmake_policy(SET CMP0064 NEW) # Support if (TEST) operator

IF (POLICY CMP0042)
    CMAKE_POLICY(SET CMP0042 NEW)
ENDIF ()





if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()



set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Os -g")

set(CMAKE_C_FLAGS "-std=c11 -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Os -g")

set(MSC_SOURCE_FILES)
set(MSC_INCLUDE_DIRS)
set(MSC_FLAGS)
set(MSC_DEPS)
set(MSC_DEFS LIBUS_USE_OPENSSL LIBUS_USE_LIBUV DEBUG)
set(LINK_OPTIONS)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(MSC_DEFS ${MSC_DEFS} DEBUG=0 MSC_DEBUG_TRACE_INSTRUCTIONS=0)
endif()

# Utility function which will only add compiler flags if supported
INCLUDE(CheckCCompilerFlag)
FUNCTION(ADD_FLAG_IF_SUPPORTED container flag)
    CHECK_C_COMPILER_FLAG(${flag} _flag_supported)
    IF (_flag_supported)
        # NOTE: Can't use LIST(APPEND...) here because it doesn't support
        # PARENT_SCOPE:
        #   http://stackoverflow.com/a/10032337
        SET(${container} ${${container}} ${flag} PARENT_SCOPE)
    ENDIF ()
ENDFUNCTION()


file(GLOB SOURCE_FILES_C src/**/*.c)
file(GLOB SOURCE_FILES_H src/**/*.h)
file(GLOB SOURCE_FILES_INC src/**/*.inc)

LIST(APPEND MSC_SOURCE_FILES ${SOURCE_FILES_C} ${SOURCE_FILES_H} ${SOURCE_FILES_INC})

LIST(APPEND MSC_INCLUDE_DIRS
        src/cli
        src/packages
        )

#include mosc dependancy
LIST(APPEND MSC_INCLUDE_DIRS
        deps/mosc/api
        deps/mosc/src/builtin
        deps/mosc/src/common
        deps/mosc/src/compiler
        deps/mosc/src/helpers
        deps/mosc/src/memory
        deps/mosc/src/meta
        deps/mosc/src/runtime
        )


file(GLOB SOURCE_FILES_C deps/mosc/src/**/*.c)
file(GLOB SOURCE_FILES_H deps/mosc/src/**/*.h)
file(GLOB SOURCE_FILES_INC deps/mosc/src/**/*.inc)

LIST(APPEND MSC_SOURCE_FILES ${SOURCE_FILES_C} ${SOURCE_FILES_H} ${SOURCE_FILES_INC})
LIST(APPEND MSC_SOURCE_FILES deps/mosc/api/msc.h)

#include nafamaw dependancy

LIST(APPEND MSC_INCLUDE_DIRS
        deps/nafamaw
        deps/nafamaw/vendors
        deps/nafamaw/packages
        )

file(GLOB SOURCE_FILES_C deps/nafamaw/**/*.c)
file(GLOB SOURCE_FILES_H deps/nafamaw/**/*.h)
file(GLOB SOURCE_FILES_INC deps/nafamaw/**/*.inc)

LIST(APPEND MSC_SOURCE_FILES ${SOURCE_FILES_C} ${SOURCE_FILES_H} ${SOURCE_FILES_INC})
LIST(APPEND MSC_SOURCE_FILES deps/nafamaw/nafamaw.c deps/nafamaw/nafamaw.h)


#include libuv dependancy
# include_directories(deps/libuv/include deps/libuv/src)

file(GLOB SOURCE_FILES_H deps/libuv/include/**.h)
file(GLOB SOURCE_FILES_C deps/libuv/src/*.c)
file(GLOB SOURCE_FILES_CH deps/libuv/src/*.h)

#LIST(APPEND MSC_SOURCE_FILES ${SOURCE_FILES_H})
#LIST(APPEND MSC_SOURCE_FILES ${SOURCE_FILES_C})
LIST(APPEND MSC_SOURCE_FILES ${SOURCE_FILES_C})

LIST(APPEND MSC_INCLUDE_DIRS deps/libuv/include deps/libuv/src)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
LIST(APPEND MSC_DEPS ${OPENSSL_LIBRARIES} OpenSSL::SSL z stdc++)

if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif ()


IF (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    LIST(APPEND MSC_DEFS WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0600)
    LIST(APPEND MSC_INCLUDE_DIRS
            deps/libuv/src/win
            )
    LIST(APPEND MSC_DEPS imm32 winmm version wldap32 ws2_32 psapi iphlpapi userenv)
    LIST(APPEND MSC_SOURCE_FILES
            deps/libuv/src/win/*.h
            deps/libuv/src/win/*.c
            )

ELSE ()
    ADD_FLAG_IF_SUPPORTED(MSC_FLAGS -fvisibility=hidden)
    # ADD_FLAG_IF_SUPPORTED(MSC_FLAGS -std=c++17)
    ADD_FLAG_IF_SUPPORTED(MSC_FLAGS -pedantic)
    ADD_FLAG_IF_SUPPORTED(MSC_FLAGS -Wall)
    ADD_FLAG_IF_SUPPORTED(MSC_FLAGS -Wextra)
    ADD_FLAG_IF_SUPPORTED(MSC_FLAGS -Wno-unused-parameter)
    LIST(APPEND MSC_DEFS
            _LARGEFILE_SOURCE
            _FILE_OFFSET_BITS=64
            )
    LIST(APPEND MSC_SOURCE_FILES
            deps/libuv/src/unix/async.c
            deps/libuv/src/unix/core.c
            deps/libuv/src/unix/dl.c
            deps/libuv/src/unix/fs.c
            deps/libuv/src/unix/getaddrinfo.c
            deps/libuv/src/unix/getnameinfo.c
            deps/libuv/src/unix/loop-watcher.c
            deps/libuv/src/unix/loop.c
            deps/libuv/src/unix/pipe.c
            deps/libuv/src/unix/poll.c
            deps/libuv/src/unix/process.c
            deps/libuv/src/unix/random-devurandom.c
            deps/libuv/src/unix/signal.c
            deps/libuv/src/unix/stream.c
            deps/libuv/src/unix/tcp.c
            deps/libuv/src/unix/thread.c
            deps/libuv/src/unix/tty.c
            deps/libuv/src/unix/udp.c)

    LIST(APPEND MSC_INCLUDE_DIRS deps/libuv/src/unix)
    FIND_PACKAGE(Threads REQUIRED)
    LIST(APPEND MSC_DEPS ${CMAKE_THREAD_LIBS_INIT})

    IF (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        LIST(APPEND MSC_DEPS
                dl)
        LIST(APPEND MSC_DEFS
                _DARWIN_USE_64_BIT_INODE=1
                _DARWIN_UNLIMITED_SELECT=1

                )
        LIST(APPEND MSC_SOURCE_FILES
                deps/libuv/src/unix/bsd-ifaddrs.c
                deps/libuv/src/unix/darwin.c
                deps/libuv/src/unix/darwin-proctitle.c
                deps/libuv/src/unix/fsevents.c
                deps/libuv/src/unix/kqueue.c
                deps/libuv/src/unix/proctitle.c
                deps/libuv/src/unix/random-getentropy.c)
    ELSEIF (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        LIST(APPEND MSC_DEFS _GNU_SOURCE _POSIX_C_SOURCE=200112)
        LIST(APPEND MSC_DEPS pthread dl m rt)
        LIST(APPEND MSC_SOURCE_FILES
                deps/libuv/src/unix/linux-core.c
                deps/libuv/src/unix/linux-inotify.c
                deps/libuv/src/unix/linux-syscalls.c
                deps/libuv/src/unix/procfs-exepath.c
                deps/libuv/src/unix/proctitle.c
                deps/libuv/src/unix/random-getrandom.c
                deps/libuv/src/unix/random-sysctl-linux.c
                deps/libuv/src/unix/epoll.c
                )
    ELSEIF (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        LIST(APPEND MSC_DEPS pthread dl m rt)
        LIST(APPEND MSC_SOURCE_FILES
                deps/libuv/src/unix/bsd-ifaddrs.c
                deps/libuv/src/unix/bsd-proctitle.c
                deps/libuv/src/unix/freebsd.c
                deps/libuv/src/unix/kqueue.c
                deps/libuv/src/unix/posix-hrtime.c
                deps/libuv/src/unix/random-getrandom.c)
    ENDIF ()
ENDIF ()



# incluse uWebsocket
LIST(APPEND MSC_INCLUDE_DIRS
        deps/uWebSockets/src
        deps/uWebSockets/capi
        deps/uWebSockets/uSockets/src
        )
# add uSocket source
file(GLOB SOURCE_FILES_C deps/uWebSockets/uSockets/src/*.c deps/uWebSockets/uSockets/src/eventing/*.c deps/uWebSockets/uSockets/src/crypto/*.c deps/uWebSockets/uSockets/src/io_uring/*.c deps/uWebSockets/uSockets/src/crypto/*.cpp)
LIST(APPEND MSC_SOURCE_FILES ${SOURCE_FILES_C})
# add uWebsocket source
file(GLOB SOURCE_FILES_C deps/uWebSockets/capi/*.cpp)
LIST(APPEND MSC_SOURCE_FILES ${SOURCE_FILES_C})


ADD_EXECUTABLE(moscc ${MSC_SOURCE_FILES})
TARGET_COMPILE_DEFINITIONS(moscc PRIVATE ${MSC_DEFS})
TARGET_INCLUDE_DIRECTORIES(moscc PUBLIC ${MSC_INCLUDE_DIRS})
# SET_PROPERTY(TARGET moscc PROPERTY C_STANDARD 11)
# TARGET_LINK_OPTIONS(moscc PRIVATE ${LINK_OPTIONS})
TARGET_COMPILE_OPTIONS(moscc PRIVATE ${MSC_FLAGS})
TARGET_LINK_LIBRARIES(moscc PRIVATE ${MSC_DEPS})

set_target_properties(moscc PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin/
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/bin/
        )

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set_target_properties(moscc PROPERTIES SUFFIX "_d")
endif()

