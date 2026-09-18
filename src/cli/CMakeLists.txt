add_executable(kernelmgr_cli
        kernelmgr_cli.cpp
        Parser.h
        Prompt.h
        Completion.h
        Ink.h
        Listing.h
        Reporter.h
        Stages.h
        ${SANITIZER_SOURCES})

target_compile_options(kernelmgr_cli PRIVATE ${COMPILE_FLAGS})

# A report name per binary, so two binaries sharing a directory are not told apart by pid alone.
if (SANITIZE)
    target_compile_definitions(kernelmgr_cli PRIVATE
            KERNELMGR_SANITIZER_LOG="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kmgr")
endif ()

target_link_libraries(kernelmgr_cli PRIVATE
        kernelmgr_core
        kernelmgr_sanitizers)

set_target_properties(kernelmgr_cli PROPERTIES OUTPUT_NAME kmgr)
