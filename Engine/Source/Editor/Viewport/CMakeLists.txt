
set(MODULE_NAME WindEffects-Viewport)

set(SOURCE_FILES
    Private/GamePanel.cpp
    Private/ViewportModule.cpp
    Private/ViewportPanel.cpp
    Private/ViewportToolbarState.cpp
    Private/ViewportNavigation.cpp
    Private/ViewportNavigationPreferences.cpp
    Private/Widgets/ViewportOverlay.cpp
    Private/Widgets/ViewportSliderPopup.cpp
    Private/Widgets/ViewportWidget.cpp
)

file(GLOB_RECURSE PUBLIC_HEADERS
    "Public/*.hpp"
)

add_library(${MODULE_NAME} SHARED ${SOURCE_FILES} ${PUBLIC_HEADERS})
set_target_properties(${MODULE_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_include_directories(${MODULE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")


target_link_libraries(${MODULE_NAME} PUBLIC 
    WindEffects-Core
    WindEffects-Application
    WindEffects-ToolsPanel
    WindEffects-PlaceActors
    WindEffects-Toolbar
    WindEffects-Engine
    WindEffects-Scene
    WindEffects-Renderer 
    glm 
    imgui 
    volk 
    SDL3::SDL3
)
