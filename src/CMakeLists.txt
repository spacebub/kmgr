include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(core)
add_subdirectory(cli)
if (GUI)
    add_subdirectory(gui)
endif ()
