set(PROGRAM_NAME WindEffectsEditor)

set(SOURCE_FILES
    Main.cpp
    Editor.cpp
    Editor.hpp
    FirstRunAgreementPopup.cpp
    FirstRunAgreementPopup.hpp
    ModuleBootstrap.cpp
    Resource.rc
)

add_executable(${PROGRAM_NAME} ${SOURCE_FILES})

# Link modules
target_link_libraries(${PROGRAM_NAME} PRIVATE 
    WindEffects-Core
    WindEffects-Application
    WindEffects-Engine
    WindEffects-Renderer
    WindEffects-Scene
    WindEffects-World
    WindEffects-MainFrame
    WindEffects-Viewport
    WindEffects-Docking
    WindEffects-ContentBrowser
    WindEffects-WorldOutliner
    WindEffects-PropertyEditor
    WindEffects-Toolbar
    WindEffects-Menus
    WindEffects-Details
    WindEffects-ToolsPanel
    WindEffects-PlaceActors
    WindEffects-Environment
    WindEffects-EditorGridRenderer
    SDL3::SDL3
    delayimp
)

# Delay load feature modules so they can be placed in Modules/ and loaded by ModuleManager
target_link_options(${PROGRAM_NAME} PRIVATE
    "/DELAYLOAD:WindEffects-MainFrame.dll"
    "/DELAYLOAD:WindEffects-Viewport.dll"
    "/DELAYLOAD:WindEffects-Docking.dll"
    "/DELAYLOAD:WindEffects-ContentBrowser.dll"
    "/DELAYLOAD:WindEffects-WorldOutliner.dll"
    "/DELAYLOAD:WindEffects-PropertyEditor.dll"
    "/DELAYLOAD:WindEffects-Toolbar.dll"
    "/DELAYLOAD:WindEffects-Menus.dll"
    "/DELAYLOAD:WindEffects-Details.dll"
    "/DELAYLOAD:WindEffects-ToolsPanel.dll"
    "/DELAYLOAD:WindEffects-PlaceActors.dll"
    "/DELAYLOAD:WindEffects-Environment.dll"
    "/include:__pfnDliNotifyHook2"
    "/include:__pfnDliFailureHook2"
)

target_include_directories(${PROGRAM_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/Engine/Source")

# -----------------------------------------------------------------------------
# Shader compilation (HLSL -> SPIR-V via root cmake/ShaderPipeline.cmake)
# -----------------------------------------------------------------------------
add_dependencies(${PROGRAM_NAME} WindEffects_CompileShaders)

set(SHADER_OUTPUT_DIR "${WE_SHADER_OUTPUT_DIR}")

# Create clean runtime layout and copy assets
add_custom_command(TARGET ${PROGRAM_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Assets/Shaders"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Assets/Fonts"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Assets/Icons"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Assets/Editor"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Config/Editor"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/Engine/Config/Editor" "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Config/Editor"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Legal"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/LICENSE" "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/LICENSE"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/Legal/EULA.md" "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Legal/EULA.md"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/Legal/THIRD_PARTY_NOTICES.md" "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Legal/THIRD_PARTY_NOTICES.md"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Saved/logs"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Plugins"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Modules"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/ThirdParty"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SHADER_OUTPUT_DIR}" "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Assets/Shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/Engine/Content/Fonts" "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Assets/Fonts"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/Engine/Content/Icons" "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Assets/Icons"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/Assets/Editor" "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Assets/Editor"
    COMMENT "Setting up clean runtime layout (Assets, Config, Saved, Plugins, Modules)"
)

add_custom_command(TARGET ${PROGRAM_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -DRUNTIME_DIR=$<TARGET_FILE_DIR:${PROGRAM_NAME}>
        -P "${CMAKE_SOURCE_DIR}/cmake/CleanupRuntimeDir.cmake"
    COMMENT "Removing CMake build artifacts from runtime output"
)

# Feature modules live only in Modules/. Remove stale root copies from older builds.
set(FEATURE_MODULES
    WindEffects-MainFrame
    WindEffects-Viewport
    WindEffects-Docking
    WindEffects-ContentBrowser
    WindEffects-WorldOutliner
    WindEffects-PropertyEditor
    WindEffects-Toolbar
    WindEffects-Menus
    WindEffects-Details
    WindEffects-ToolsPanel
    WindEffects-PlaceActors
    WindEffects-Environment
)
foreach(MOD IN LISTS FEATURE_MODULES)
    if(TARGET ${MOD})
        add_dependencies(${PROGRAM_NAME} ${MOD})
    endif()

    add_custom_command(TARGET ${PROGRAM_NAME} POST_BUILD
        COMMAND cmd /c "if exist \"$<TARGET_FILE_DIR:${PROGRAM_NAME}>\\${MOD}.dll\" del /f /q \"$<TARGET_FILE_DIR:${PROGRAM_NAME}>\\${MOD}.dll\""
        COMMENT "Removing stale ${MOD}.dll from editor root"
    )
endforeach()

# Runtime modules and third-party DLLs must live next to the editor executable.
set(RUNTIME_DLL_TARGETS
    WindEffects-Core
    WindEffects-Application
    WindEffects-Engine
    WindEffects-Renderer
    WindEffects-Scene
    WindEffects-World
    WindEffects-RHI
)
foreach(MOD IN LISTS RUNTIME_DLL_TARGETS)
    if(TARGET ${MOD})
        add_dependencies(${PROGRAM_NAME} ${MOD})
        add_custom_command(TARGET ${PROGRAM_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${MOD}>"
                "$<TARGET_FILE_DIR:${PROGRAM_NAME}>"
            COMMENT "Deploying runtime module ${MOD} next to editor"
        )
    endif()
endforeach()

if(TARGET SDL3::SDL3)
    add_custom_command(TARGET ${PROGRAM_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SDL3::SDL3>"
            "$<TARGET_FILE_DIR:${PROGRAM_NAME}>"
        COMMENT "Deploying SDL3.dll next to editor"
    )
elseif(TARGET SDL3-shared)
    add_custom_command(TARGET ${PROGRAM_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SDL3-shared>"
            "$<TARGET_FILE_DIR:${PROGRAM_NAME}>"
        COMMENT "Deploying SDL3.dll next to editor"
    )
endif()

# Stale artifact: EditorGridRenderer is linked statically now.
add_custom_command(TARGET ${PROGRAM_NAME} POST_BUILD
    COMMAND cmd /c "if exist \"$<TARGET_FILE_DIR:${PROGRAM_NAME}>\\Modules\\WindEffects-EditorGridRenderer.dll\" del /f /q \"$<TARGET_FILE_DIR:${PROGRAM_NAME}>\\Modules\\WindEffects-EditorGridRenderer.dll\""
    COMMENT "Removing stale WindEffects-EditorGridRenderer.dll from Modules/"
)
