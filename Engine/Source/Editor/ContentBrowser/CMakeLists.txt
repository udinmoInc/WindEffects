
set(MODULE_NAME WindEffects-ContentBrowser)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    "Private/*.cpp"
    "Private/*.hpp"
)

list(APPEND SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Private/Services/ContentBrowserFolderArt.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Private/Services/ContentBrowserBlueprintArt.cpp"
)

# Add PCH
list(APPEND SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../EditorPCH.cpp"
)

list(REMOVE_DUPLICATES SOURCE_FILES)

file(GLOB_RECURSE PUBLIC_HEADERS
    "Public/*.hpp"
)

add_library(${MODULE_NAME} SHARED ${SOURCE_FILES} ${PUBLIC_HEADERS})
set_target_properties(${MODULE_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Use target-based APIs
target_include_directories(${MODULE_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Public>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${stb_SOURCE_DIR}
    ${nanosvg_SOURCE_DIR}/src
)

target_compile_features(${MODULE_NAME} PUBLIC cxx_std_23)

# Setup PCH
we_setup_pch(${MODULE_NAME} "EditorPCH.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../EditorPCH.cpp")

target_link_libraries(${MODULE_NAME} PUBLIC
    WindEffects-Core
    WindEffects-Application
    WindEffects-Renderer
    glm
    volk
    SDL3::SDL3
)

add_dependencies(${MODULE_NAME} WindEffects-Application)

# Unity build
we_setup_unity_build(${MODULE_NAME})
