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
# Shader compilation (GLSL -> SPIR-V)
# -----------------------------------------------------------------------------
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Engine/Source/Runtime/Shaders")
set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/Generated/Shaders")
file(MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}")

find_program(GLSLC_EXECUTABLE NAMES glslc HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin")

if(NOT GLSLC_EXECUTABLE)
    file(GLOB VULKAN_SDK_DIRS "C:/VulkanSDK/*")
    foreach(SDK_DIR IN LISTS VULKAN_SDK_DIRS)
        if(EXISTS "${SDK_DIR}/Bin/glslc.exe")
            set(GLSLC_EXECUTABLE "${SDK_DIR}/Bin/glslc.exe")
            break()
        endif()
    endforeach()
endif()

set(SHADER_GLSL_SOURCES
    "${SHADER_SOURCE_DIR}/UI.vert"
    "${SHADER_SOURCE_DIR}/UI.frag"
    "${SHADER_SOURCE_DIR}/Grid.vert"
    "${SHADER_SOURCE_DIR}/Grid.frag"
    "${SHADER_SOURCE_DIR}/Skybox.vert"
    "${SHADER_SOURCE_DIR}/Skybox.frag"
    "${SHADER_SOURCE_DIR}/SceneObject.vert"
    "${SHADER_SOURCE_DIR}/SceneObject.frag"
)

set(SHADER_SPV_OUTPUTS "")
foreach(SHADER_SOURCE ${SHADER_GLSL_SOURCES})
    if(EXISTS "${SHADER_SOURCE}")
        get_filename_component(SHADER_FILENAME "${SHADER_SOURCE}" NAME)
        set(SPV_OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_FILENAME}.spv")
        if(GLSLC_EXECUTABLE)
            add_custom_command(
                OUTPUT "${SPV_OUTPUT}"
                COMMAND "${GLSLC_EXECUTABLE}" "${SHADER_SOURCE}" -o "${SPV_OUTPUT}"
                DEPENDS "${SHADER_SOURCE}"
                COMMENT "Compiling shader ${SHADER_FILENAME}"
                VERBATIM
            )
            list(APPEND SHADER_SPV_OUTPUTS "${SPV_OUTPUT}")
        endif()
    endif()
endforeach()

if(NOT GLSLC_EXECUTABLE)
    message(WARNING "glslc not found. Install Vulkan SDK (set VULKAN_SDK) to compile shaders. Existing SPIR-V in ${SHADER_OUTPUT_DIR} will be used if present.")
endif()

if(GLSLC_EXECUTABLE AND SHADER_SPV_OUTPUTS)
    add_custom_target(WindEffects_CompileShaders DEPENDS ${SHADER_SPV_OUTPUTS})
    add_dependencies(${PROGRAM_NAME} WindEffects_CompileShaders)
endif()

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
