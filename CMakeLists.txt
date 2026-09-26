cmake_minimum_required(VERSION 3.25)
project(kernelmgr
        VERSION 1.0.2
        DESCRIPTION "Provides a tool for kernel building and management"
        HOMEPAGE_URL "https://github.com/spacebub/kernelmgr")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(GUI "Build graphical interface" ON)

option(SANITIZE "Build with the address and undefined sanitizers" OFF)

add_compile_definitions(KERNELMGR_VERSION="${PROJECT_VERSION}")

set(COMPILE_FLAGS
        "-Wall"
        "-Wextra"
        "-Werror"
        "-Wpedantic"
        "$<$<CONFIG:DEBUG>:-Og>")

add_library(kernelmgr_sanitizers INTERFACE)

if (SANITIZE)
    set(SANITIZER_FLAGS -fsanitize=address -fsanitize=undefined)

    # No -Og: it folds frames together and points sanitizer stacks at the wrong function.
    target_compile_options(kernelmgr_sanitizers INTERFACE
            ${SANITIZER_FLAGS}
            -fno-omit-frame-pointer
            -fno-optimize-sibling-calls
            -g)

    target_link_options(kernelmgr_sanitizers INTERFACE ${SANITIZER_FLAGS})

    # The runtimes read their options off hooks compiled into the binary. The hooks
    # define symbols, so they go into each binary, never the interface library that
    # every linked library would also carry.
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/lsan.supp SANITIZER_SUPPRESSIONS)
    configure_file(cmake/Sanitizers.cpp.in ${CMAKE_BINARY_DIR}/kernelmgr_sanitizers.cpp @ONLY)

    set(SANITIZER_SOURCES ${CMAKE_BINARY_DIR}/kernelmgr_sanitizers.cpp)
endif ()

include(FetchContent)

# A checkout on disk stands in for the fetch:
#   cmake -B build -DFETCHCONTENT_SOURCE_DIR_TINYTK=/path/to/tinytk
set(KERNELMGR_TINYTK_VERSION v1.1.2)

set(TTK_UI ${GUI} CACHE BOOL "" FORCE)
set(TTK_HTTP OFF CACHE BOOL "" FORCE)
set(TTK_NOTIFY ${GUI} CACHE BOOL "" FORCE)
set(TTK_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TTK_GALLERY OFF CACHE BOOL "" FORCE)
set(TTK_BENCHMARKS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(tinytk
        GIT_REPOSITORY https://github.com/spacebub/tinytk.git
        GIT_TAG ${KERNELMGR_TINYTK_VERSION}
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
        SYSTEM)

FetchContent_MakeAvailable(tinytk)

add_subdirectory(src)
