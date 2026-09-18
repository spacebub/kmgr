add_executable(kernelmgr_gui
        kernelmgr_gui.cpp
        # app: the window and what it is made of
        app/App.cpp
        app/App.h
        app/Desk.cpp
        app/Desk.h
        app/Frame.cpp
        app/Frame.h
        app/Look.cpp
        app/Look.h
        app/Reach.h
        app/TitleBar.cpp
        app/TitleBar.h
        # model: what the pages read, with no window in it
        model/Catalog.cpp
        model/Catalog.h
        model/Format.h
        model/Logs.cpp
        model/Logs.h
        model/SettingsBridge.cpp
        model/SettingsBridge.h
        model/SystemStatus.cpp
        model/SystemStatus.h
        model/WorkflowRunner.cpp
        model/WorkflowRunner.h
        # services: what owns a thread or talks to the desktop
        services/DesktopAlert.cpp
        services/DesktopAlert.h
        services/Watcher.cpp
        services/Watcher.h
        # components: what the pages are built from
        components/BuildCard.cpp
        components/BuildCard.h
        components/Buzzes.h
        components/Choice.cpp
        components/Choice.h
        components/EmptyState.h
        components/Fold.h
        components/HoldingRow.cpp
        components/HoldingRow.h
        components/InstalledCard.cpp
        components/InstalledCard.h
        components/LogSteps.cpp
        components/LogSteps.h
        components/Parts.cpp
        components/Parts.h
        components/PathField.cpp
        components/PathField.h
        components/ProgressTrack.cpp
        components/ProgressTrack.h
        components/SectionTabs.cpp
        components/SectionTabs.h
        components/SettingRow.cpp
        components/SettingRow.h
        components/VersionList.cpp
        components/VersionList.h
        components/Wash.h
        # pages: one per page the user can be on
        pages/HomePage.cpp
        pages/HomePage.h
        pages/KernelsPage.cpp
        pages/KernelsPage.h
        pages/SettingsPage.cpp
        pages/SettingsPage.h
        # overlays: what takes the window over
        overlays/WorkflowSheet.cpp
        overlays/WorkflowSheet.h
        ${SANITIZER_SOURCES})

target_link_libraries(kernelmgr_gui PRIVATE
        kernelmgr_core
        kernelmgr_sanitizers
        ttk::ui)

target_compile_options(kernelmgr_gui PRIVATE ${COMPILE_FLAGS})

ttk_embed(kernelmgr_gui mark128 ${CMAKE_SOURCE_DIR}/assets/kernelmgr-128.png)
ttk_embed(kernelmgr_gui mark48 ${CMAKE_SOURCE_DIR}/assets/kernelmgr-48.png)

if (SANITIZE)
    target_compile_definitions(kernelmgr_gui PRIVATE
            KERNELMGR_SANITIZER_LOG="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/KernelManager")
endif ()

set_target_properties(kernelmgr_gui PROPERTIES OUTPUT_NAME KernelManager)
