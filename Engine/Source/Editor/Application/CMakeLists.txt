
set(MODULE_NAME WindEffects-Application)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    "Private/*.cpp"
    "Private/*.hpp"
)

file(GLOB_RECURSE PUBLIC_HEADERS CONFIGURE_DEPENDS
    "Public/*.hpp"
)

# Ensure IconManager is always compiled (used by ContentBrowser via Application DLL exports).
list(APPEND SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/IconManager.cpp"
)

# Add PCH
list(APPEND SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../EditorPCH.cpp"
)

list(REMOVE_DUPLICATES SOURCE_FILES)

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
    WindEffects-Renderer
    glm
    volk
    SDL3::SDL3
)

# Unity build
we_setup_unity_build(${MODULE_NAME})
