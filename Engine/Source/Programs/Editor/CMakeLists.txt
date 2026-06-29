set(PROGRAM_NAME WindEffectsEditor)

file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.hpp" "*.rc")

add_executable(${PROGRAM_NAME} ${SOURCE_FILES})

# Link modules
target_link_libraries(${PROGRAM_NAME} PRIVATE 
    WindEffects-Core
    WindEffects-Application
    WindEffects-Engine
    WindEffects-Renderer
    WindEffects-Scene
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
    SDL3::SDL3
    delayimp
)

# Delay load feature modules so they can be placed in Modules/ and loaded by ModuleManager
set_property(TARGET ${PROGRAM_NAME} APPEND_STRING PROPERTY LINK_FLAGS 
    " /DELAYLOAD:WindEffects-MainFrame.dll"
    " /DELAYLOAD:WindEffects-Viewport.dll"
    " /DELAYLOAD:WindEffects-Docking.dll"
    " /DELAYLOAD:WindEffects-ContentBrowser.dll"
    " /DELAYLOAD:WindEffects-WorldOutliner.dll"
    " /DELAYLOAD:WindEffects-PropertyEditor.dll"
    " /DELAYLOAD:WindEffects-Toolbar.dll"
    " /DELAYLOAD:WindEffects-Menus.dll"
    " /DELAYLOAD:WindEffects-Details.dll"
    " /DELAYLOAD:WindEffects-ToolsPanel.dll"
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
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Config"
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

# Linked feature-module DLLs must live next to the editor exe: Windows resolves
# import-table dependencies before main(). Modules/ is an additional copy for
# ModuleManager::LoadModule.
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
)
foreach(MOD IN LISTS FEATURE_MODULES)
    if(TARGET ${MOD})
        add_custom_command(TARGET ${PROGRAM_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${MOD}>"
                "$<TARGET_FILE_DIR:${PROGRAM_NAME}>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${MOD}>"
                "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/Modules"
            COMMENT "Deploying ${MOD} next to editor and under Modules/"
        )
    endif()
endforeach()
